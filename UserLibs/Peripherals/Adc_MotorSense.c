/**
 * @file    Adc_MotorSense.c
 * @brief   10-channel unified TMADC + DMA motor sensing
 *
 * Synchronized S&H freeze mechanism (reference manual section 45.4.2):
 * ─────────────────────────────────────────────────────────────────────────
 *  "Each external analog input channel of a TMADC has its dedicated
 *   sample and hold (SH) circuitry, therefore allowing independent SH
 *   operation on each channel."
 *
 *  "A synchronous end of sampling on a channel group can be obtained by
 *   configuring the channel group on the same trigger source."
 *
 *  Implementation: every channel is initialized with a pointer to the
 *  SAME grpCfg struct. The iLLD writes identical HWTRxMS values to every
 *  CHy_CFG register. When the trigger fires, all 10 S&H switches open at
 *  the same clock edge — a true simultaneous hardware snapshot.
 *
 *  Additionally, HWTRDLYEN = 0 is forced on all channels after iLLD init
 *  to guarantee no per-channel delay offsets the freeze instant.
 *
 * DMA flow:
 * ─────────────────────────────────────────────────────────────────────────
 *  SR_0 fires on each channel result event (OR logic, no WFC).
 *  DMA does one 32-bit transfer per SR fire (CHCTR.TREL = 10).
 *  After 10 fires: DMA raises CPU ISR at IFX_INTPRIO_ADC_ALL.
 *  ISR computes differential currents, scales voltages, re-arms DMA.
 */

#include "Adc_MotorSense.h"
#include "Dma/Std/IfxDma.h"
#include "IfxCpu.h"

/*══════════════════════════════════════════════════════════════════════════
 * MODULE-LEVEL OBJECTS
 *══════════════════════════════════════════════════════════════════════════*/
static IfxAdc_Tmadc g_module;

/** Unified group handle — holds sourceAddress used for DMA re-arm */
static IfxAdc_Tmadc_Group g_grpAll;

/* Current channel handles — P-side on Core-0, N-side on Core-1 */
static IfxAdc_Tmadc_Ch g_chIa_P, g_chIa_N;
static IfxAdc_Tmadc_Ch g_chIb_P, g_chIb_N;
static IfxAdc_Tmadc_Ch g_chIc_P, g_chIc_N;

/* Voltage channel handles — alternating cores */
static IfxAdc_Tmadc_Ch g_chVa, g_chVb, g_chVc, g_chVdc;

/*
 * DMA destination buffer — 10 × 32-bit result register words.
 * 64-byte alignment satisfies DMA address alignment requirements.
 * Layout mirrors the result register / DMA index map in the header.
 */
static uint32 IFX_ALIGN(64) g_dmaAll[ADC_DMA_BUF_SIZE];

/* Processed results — written in ISR, read by application on CPU2 */
volatile Adc_PhaseCurrents g_currents;
volatile Adc_PhaseVoltages g_voltages;
volatile boolean g_allReady = FALSE;

/*══════════════════════════════════════════════════════════════════════════
 * MODULE INIT
 *══════════════════════════════════════════════════════════════════════════*/
static void prv_initModule (void)
{
    IfxAdc_Tmadc_Config modCfg;
    IfxAdc_enableModule(&MODULE_ADC);
    IfxAdc_Tmadc_initModuleConfig(&modCfg, &MODULE_ADC);

    modCfg.id = ADC_TMADC_MODULE;
    modCfg.calEnable = TRUE;

    /*
     * Service Request 0:
     *   eventOp  = orLogicWithoutWfc → fires on ANY result event from any
     *              channel connected to this SR node (one fire per channel).
     *   routed to DMA-0 → DMA does one 32-bit transfer per fire.
     *   After 10 fires (all channels converted), DMA raises CPU ISR.
     */
    IfxAdc_Tmadc_InterruptConfig sr0 = {.node = IfxAdc_TmadcServReq_0, .eventOp = IfxAdc_TmadcEventOp_orLogicWithoutWfc,
            .eventSel = IfxAdc_TmadcEventSel_result, .typeOfService = IfxSrc_Tos_dma0, .vmId = 0U, .priority =
                    (Ifx_Priority) ADC_DMA_CHANNEL};

    IfxAdc_Tmadc_ServRequestConfig srvCfg;
    srvCfg.intConfig[0] = &sr0;
    srvCfg.numServReqNodes = 1U;
    modCfg.srvReqCfg = &srvCfg;

    IfxAdc_Tmadc_initModule(&g_module, &modCfg);
}

/*══════════════════════════════════════════════════════════════════════════
 * UNIFIED GROUP INIT
 *
 * All 10 channels are placed in one group sharing one trigger source.
 * The grpCfg struct is stack-local — it is fully consumed by the iLLD
 * channel and group init calls before this function returns, so its
 * lifetime as a local variable is safe.
 *
 * CRITICAL — same grpCfg pointer for every channel:
 *   IfxAdc_Tmadc_initChannel writes the trigger mux select (HWTRxMS)
 *   from grpCfg.trigger into each channel's CHy_CFG register.
 *   If any channel were to use a different grpCfg (even with identical
 *   trigger values), there would be no guarantee of register-level
 *   identity, and the synchronized freeze could silently break.
 *══════════════════════════════════════════════════════════════════════════*/
static void prv_initUnifiedGroup (void)
{
    /*─── Trigger: same source for all 10 channels ───────────────────────
     * When this trigger fires, it ends sampling on every channel that has
     * completed its minimum sampling time (tmstc). All channels are
     * configured identically, so all freeze at the same trigger edge.
     */
    IfxAdc_Tmadc_ChannelTriggerConfig trig = {.hwTrigger1 = {ADC_TMADC_TRIG_MUXSEL, IfxAdc_TmadcTriggerMode_risingEdge},
            .hwTrigger2 = {IfxAdc_TmadcTriggerMuxSel_0, IfxAdc_TmadcTriggerMode_disable}, .delayNS =
            ADC_TRIGGER_DELAY_NS /* 0.0f — no hardware delay on any channel */
    };

    /*─── Service request: one node, OR logic ────────────────────────────
     * Each channel result fires SR_0 once → DMA transfers one 32-bit word.
     * grpSrvReq is used by IfxAdc_Tmadc_initGroup to wire all channels.
     * No per-channel channelSrvReq is needed in group mode.
     */
    IfxAdc_Tmadc_ChannelServReqConfig srvReq = {.errorNode = IfxAdc_TmadcServReq_none, .boundaryNode =
            IfxAdc_TmadcServReq_none, .resultNode = IfxAdc_TmadcServReq_0};

    /*─── DMA CPU ISR: fires after all 10 DMA transfers complete ─────────
     * This is the CPU ISR priority, NOT the DMA channel priority.
     * DMA channel priority is set by sr0.priority = ADC_DMA_CHANNEL.
     */
    IfxAdc_Tmadc_DmaSrvReq dmaSrvReq = {.typeOfService = IfxSrc_Tos_cpu0, .vmId = 0U, .priority =
            (Ifx_Priority) IFX_INTPRIO_ADC_ALL};

    IfxAdc_Tmadc_DmaConfig dmaCfg = {.channelId = ADC_DMA_CHANNEL, .dmaId = ADC_DMA_MODULE, .dmaSrvReqCfg = &dmaSrvReq};

    /*─── Group configuration ─────────────────────────────────────────────*/
    IfxAdc_Tmadc_GroupConfig grpCfg;
    IfxAdc_Tmadc_initGroupConfig(&grpCfg);

    grpCfg.channels = (uint16) ((1U << ADC_CH_IA_P) | (1U << ADC_CH_IA_N) | (1U << ADC_CH_IB_P) | (1U << ADC_CH_IB_N)
            | (1U << ADC_CH_IC_P) | (1U << ADC_CH_IC_N) | (1U << ADC_CH_VA) | (1U << ADC_CH_VB) | (1U << ADC_CH_VC)
            | (1U << ADC_CH_VDC));
    grpCfg.numChannels = ADC_DMA_BUF_SIZE; /* 10 */
    grpCfg.baseResultReg = ADC_RESREG_IA_P; /* RES[0] — 10 consecutive registers */
    grpCfg.trigger = &trig;
    grpCfg.grpSrvReq = &srvReq;
    grpCfg.mode = IfxAdc_TmadcOpMode_oneShot;
    grpCfg.waitForRead = FALSE;
    grpCfg.enableTimestamp = FALSE;
    grpCfg.moduleId = ADC_TMADC_MODULE;
    grpCfg.dmaCfg = &dmaCfg;
    grpCfg.groupResPtr = (void*) g_dmaAll;
    grpCfg.groupCallback = NULL_PTR;

    /*─────────────────────────────────────────────────────────────────────
     * CHANNEL INIT BLOCK
     *
     * cfg is re-initialized via IfxAdc_Tmadc_initChannelConfig before
     * every channel to ensure no stale fields carry over.
     *
     * cfg.groupCfg = &grpCfg is the ONLY mechanism that synchronizes S&H.
     * Do not change this to a copy or a different struct — the pointer
     * must be identical for all 10 channels.
     *────────────────────────────────────────────────────────────────────*/
    IfxAdc_Tmadc_ChConfig cfg;

    /*── Ia_P : Phase-A positive, Core-0, RES[0] ────────────────────────*/
    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_IA_P;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.core = ADC_CORE_IA_P;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.samplingTimeNS = ADC_CURR_SAMPLING_TIME_NS;
    cfg.resultCfg.resultReg = ADC_RESREG_IA_P;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg; /* ← sync anchor */
    IfxAdc_Tmadc_initChannel(&g_chIa_P, &cfg);

    /*── Ia_N : Phase-A negative, Core-1, RES[1] ────────────────────────*/
    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_IA_N;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.core = ADC_CORE_IA_N;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.samplingTimeNS = ADC_CURR_SAMPLING_TIME_NS;
    cfg.resultCfg.resultReg = ADC_RESREG_IA_N;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg; /* ← same pointer */
    IfxAdc_Tmadc_initChannel(&g_chIa_N, &cfg);

    /*── Ib_P : Phase-B positive, Core-0, RES[2] ────────────────────────*/
    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_IB_P;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.core = ADC_CORE_IB_P;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.samplingTimeNS = ADC_CURR_SAMPLING_TIME_NS;
    cfg.resultCfg.resultReg = ADC_RESREG_IB_P;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chIb_P, &cfg);

    /*── Ib_N : Phase-B negative, Core-1, RES[3] ────────────────────────*/
    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_IB_N;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.core = ADC_CORE_IB_N;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.samplingTimeNS = ADC_CURR_SAMPLING_TIME_NS;
    cfg.resultCfg.resultReg = ADC_RESREG_IB_N;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chIb_N, &cfg);

    /*── Ic_P : Phase-C positive, Core-0, RES[4] ────────────────────────*/
    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_IC_P;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.core = ADC_CORE_IC_P;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.samplingTimeNS = ADC_CURR_SAMPLING_TIME_NS;
    cfg.resultCfg.resultReg = ADC_RESREG_IC_P;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chIc_P, &cfg);

    /*── Ic_N : Phase-C negative, Core-1, RES[5] ────────────────────────*/
    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_IC_N;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.core = ADC_CORE_IC_N;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.samplingTimeNS = ADC_CURR_SAMPLING_TIME_NS;
    cfg.resultCfg.resultReg = ADC_RESREG_IC_N;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chIc_N, &cfg);

    /*── Va : Phase-A voltage, Core-0, RES[6] ───────────────────────────*/
    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_VA;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.core = ADC_CORE_VA;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.samplingTimeNS = ADC_VOLT_SAMPLING_TIME_NS;
    cfg.resultCfg.resultReg = ADC_RESREG_VA;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chVa, &cfg);

    /*── Vb : Phase-B voltage, Core-1, RES[7] ───────────────────────────*/
    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_VB;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.core = ADC_CORE_VB;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.samplingTimeNS = ADC_VOLT_SAMPLING_TIME_NS;
    cfg.resultCfg.resultReg = ADC_RESREG_VB;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chVb, &cfg);

    /*── Vc : Phase-C voltage, Core-0, RES[8] ───────────────────────────*/
    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_VC;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.core = ADC_CORE_VC;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.samplingTimeNS = ADC_VOLT_SAMPLING_TIME_NS;
    cfg.resultCfg.resultReg = ADC_RESREG_VC;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chVc, &cfg);

    /*── Vdc : DC link, Core-1, RES[9] ──────────────────────────────────*/
    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_VDC;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.core = ADC_CORE_VDC;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.samplingTimeNS = ADC_VOLT_SAMPLING_TIME_NS;
    cfg.resultCfg.resultReg = ADC_RESREG_VDC;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chVdc, &cfg);

    /*─────────────────────────────────────────────────────────────────────
     * Init group last — this finalizes DMA channel configuration
     * (transfer count = 10, source = RES[0] base address) and stores
     * g_grpAll.sourceAddress for use in the ISR re-arm.
     *────────────────────────────────────────────────────────────────────*/
    IfxAdc_Tmadc_initGroup(&g_grpAll, &grpCfg);
}

/*══════════════════════════════════════════════════════════════════════════
 * EGTM TRIGGER ROUTING
 *══════════════════════════════════════════════════════════════════════════*/
static void prv_connectEgtmTrigger (void)
{
    boolean ok = IfxEgtm_Trigger_trigToAdc(
    ADC_EGTM_CLUSTER,
    ADC_EGTM_TRIG_SOURCE,
    ADC_EGTM_TRIG_CHANNEL,
    ADC_EGTM_TRIG_SIGNAL);
    IFX_ASSERT(IFX_VERBOSE_LEVEL_ERROR, ok == TRUE);
}

/*══════════════════════════════════════════════════════════════════════════
 * PUBLIC API
 *══════════════════════════════════════════════════════════════════════════*/
void Adc_MotorSense_init (void)
{
    prv_initModule();
    prv_initUnifiedGroup();
    prv_connectEgtmTrigger();

    /*─────────────────────────────────────────────────────────────────────
     * FORCE HWTRDLYEN = 0 on ALL channels.
     *
     * The iLLD trigger config sets delayNS = 0, but the HWTRDLYEN bit
     * controls whether the hardware delay register is used at all.
     * Writing it explicitly here ensures no default or residual value
     * from a previous configuration can offset the freeze instant on
     * any individual channel.
     *
     * Even a 1-clock delay on one channel relative to the others would
     * corrupt the differential subtraction at 200 kHz signal rates.
     *────────────────────────────────────────────────────────────────────*/
    MODULE_ADC.TMADC[0].CH[ADC_CH_IA_P].CFG.B.HWTRDLYEN = 0;
    MODULE_ADC.TMADC[0].CH[ADC_CH_IA_N].CFG.B.HWTRDLYEN = 0;
    MODULE_ADC.TMADC[0].CH[ADC_CH_IB_P].CFG.B.HWTRDLYEN = 0;
    MODULE_ADC.TMADC[0].CH[ADC_CH_IB_N].CFG.B.HWTRDLYEN = 0;
    MODULE_ADC.TMADC[0].CH[ADC_CH_IC_P].CFG.B.HWTRDLYEN = 0;
    MODULE_ADC.TMADC[0].CH[ADC_CH_IC_N].CFG.B.HWTRDLYEN = 0;
    MODULE_ADC.TMADC[0].CH[ADC_CH_VA].CFG.B.HWTRDLYEN = 0;
    MODULE_ADC.TMADC[0].CH[ADC_CH_VB].CFG.B.HWTRDLYEN = 0;
    MODULE_ADC.TMADC[0].CH[ADC_CH_VC].CFG.B.HWTRDLYEN = 0;
    MODULE_ADC.TMADC[0].CH[ADC_CH_VDC].CFG.B.HWTRDLYEN = 0;
}

void Adc_MotorSense_start (void)
{
    IfxAdc_Tmadc_runModule(&g_module);
    /* Block until calibration completes and module is in RUN state */
    while (g_module.state != IfxAdc_Tmadc_moduleState_run)
    {
    }
}

const Adc_PhaseCurrents* Adc_MotorSense_getCurrents (void)
{
    return (const Adc_PhaseCurrents*) &g_currents;
}

const Adc_PhaseVoltages* Adc_MotorSense_getVoltages (void)
{
    return (const Adc_PhaseVoltages*) &g_voltages;
}

/*══════════════════════════════════════════════════════════════════════════
 * UNIFIED ISR — called from Adc_MotorSense_Isr.c
 *
 * Execution context:
 *   - Runs on CPU0 at priority IFX_INTPRIO_ADC_ALL
 *   - Fires after DMA has transferred all 10 result words into g_dmaAll
 *   - g_dmaAll is coherent at entry — no cache flush needed (DSPR is
 *     cache-inhibited on TC4Dx; verify for your linker section)
 *══════════════════════════════════════════════════════════════════════════*/
void Adc_MotorSense_allIsr (void)
{
    /*─────────────────────────────────────────────────────────────────────
     * VF VALIDATION
     *   Check the Valid Flag in the last result word for each core.
     *   Core-0 last = Vc  (RES[8], index 8)
     *   Core-1 last = Vdc (RES[9], index 9)
     *
     *   If VF is 0, a conversion was still in progress when DMA fired —
     *   this indicates a timing violation (tperiod too short, tmstc too
     *   long, or tHOLD exceeded). Treat as a hard fault in production code.
     *────────────────────────────────────────────────────────────────────*/
    IFX_ASSERT(IFX_VERBOSE_LEVEL_ERROR, ADC_GET_TRE(g_dmaAll[ADC_DMA_IDX_VC]) == 1U);
    IFX_ASSERT(IFX_VERBOSE_LEVEL_ERROR, ADC_GET_TRE(g_dmaAll[ADC_DMA_IDX_VDC]) == 1U);

    /*─────────────────────────────────────────────────────────────────────
     * RESULT EXTRACTION
     *   32-bit DMA word format: bits[15:4] = 12-bit result, bit[19] = VF
     *   ADC_GET_RESULT shifts and masks to give unsigned 12-bit [0..4095]
     *────────────────────────────────────────────────────────────────────*/
    const uint16 ia_p = (uint16) ADC_GET_RESULT(g_dmaAll[ADC_DMA_IDX_IA_P]);
    const uint16 ia_n = (uint16) ADC_GET_RESULT(g_dmaAll[ADC_DMA_IDX_IA_N]);
    const uint16 ib_p = (uint16) ADC_GET_RESULT(g_dmaAll[ADC_DMA_IDX_IB_P]);
    const uint16 ib_n = (uint16) ADC_GET_RESULT(g_dmaAll[ADC_DMA_IDX_IB_N]);
    const uint16 ic_p = (uint16) ADC_GET_RESULT(g_dmaAll[ADC_DMA_IDX_IC_P]);
    const uint16 ic_n = (uint16) ADC_GET_RESULT(g_dmaAll[ADC_DMA_IDX_IC_N]);

    const uint16 va_raw = (uint16) ADC_GET_RESULT(g_dmaAll[ADC_DMA_IDX_VA]);
    const uint16 vb_raw = (uint16) ADC_GET_RESULT(g_dmaAll[ADC_DMA_IDX_VB]);
    const uint16 vc_raw = (uint16) ADC_GET_RESULT(g_dmaAll[ADC_DMA_IDX_VC]);
    const uint16 vdc_raw = (uint16) ADC_GET_RESULT(g_dmaAll[ADC_DMA_IDX_VDC]);

    /*─────────────────────────────────────────────────────────────────────
     * DIFFERENTIAL SUBTRACTION — TLE4972 current sensing
     *
     *   diff = P_raw - N_raw, result is signed int16.
     *
     *   Physics:
     *     At  0 A : P = N ≈ 2048 → diff = 0
     *     At +Imax: P → 4095, N → 0    → diff = +4095
     *     At −Imax: P → 0,    N → 4095 → diff = −4095
     *
     *   Common-mode noise (supply ripple, EMI) appears equally on P and N,
     *   so it cancels in the subtraction — this is the key benefit of
     *   using the differential TLE4972 outputs.
     *────────────────────────────────────────────────────────────────────*/
    const sint16 diff_a = (sint16) ia_p - (sint16) ia_n;
    const sint16 diff_b = (sint16) ib_p - (sint16) ib_n;
    const sint16 diff_c = (sint16) ic_p - (sint16) ic_n;

    /*─── Store raw values (debug, scope, common-mode monitoring) ───*/
    g_currents.raw_p[0] = (sint16) ia_p;
    g_currents.raw_p[1] = (sint16) ib_p;
    g_currents.raw_p[2] = (sint16) ic_p;
    g_currents.raw_n[0] = (sint16) ia_n;
    g_currents.raw_n[1] = (sint16) ib_n;
    g_currents.raw_n[2] = (sint16) ic_n;
    g_currents.raw_diff[0] = diff_a;
    g_currents.raw_diff[1] = diff_b;
    g_currents.raw_diff[2] = diff_c;

    /*─── Scale to Amperes ───*/
    g_currents.ia = 0.04540246 * diff_a - 1.074525;
    g_currents.ib = 0.04535147 * diff_b - 0.9750567;
    g_currents.ic = 0.04672889 * diff_c - 0.6619926;

    /*─────────────────────────────────────────────────────────────────────
     * KIRCHHOFF CHECK: ia + ib + ic ≈ 0 for a balanced 3-phase load.
     * A large persistent error signals a sensor fault, open phase,
     * or miscalibrated ADC_CURR_SENS_MV_PER_A.
     *────────────────────────────────────────────────────────────────────*/
    g_currents.kirchhoff_err = g_currents.ia + g_currents.ib + g_currents.ic;

    /*─── Scale voltages ───*/
    g_voltages.va = va_raw;
    g_voltages.vb = vb_raw;
    g_voltages.vc = vc_raw;
    g_voltages.vdc = vdc_raw;

    /*─────────────────────────────────────────────────────────────────────
     * SIGNAL APPLICATION LAYER
     * Set flag AFTER all data is written to avoid partial-read race.
     * Application on CPU2 must treat this as a release fence.
     *────────────────────────────────────────────────────────────────────*/
    g_allReady = TRUE;

    /*─────────────────────────────────────────────────────────────────────
     * RE-ARM DMA for next PWM cycle
     *
     * Reset destination address to start of g_dmaAll so the next 10
     * transfers overwrite from index 0 again.
     * g_grpAll.sourceAddress holds the base address of RES[0] as set
     * by IfxAdc_Tmadc_initGroup — this never changes.
     * IfxDma_enableChannelTransaction re-enables hardware requests;
     * in continuous DMA mode the transfer counter auto-reloads to 10.
     *────────────────────────────────────────────────────────────────────*/
    IfxDma_setChannelDestinationAddress(&MODULE_DMA0, ADC_DMA_CHANNEL, (void*) g_dmaAll);
    IfxDma_setChannelSourceAddress(&MODULE_DMA0, ADC_DMA_CHANNEL, (const void*) g_grpAll.sourceAddress);
    IfxDma_enableChannelTransaction(&MODULE_DMA0, ADC_DMA_CHANNEL);
}
