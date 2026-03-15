/**
 * @file    Adc_MotorSense.c
 * @brief   7-channel TMADC + DMA motor sensing
 *          3 phase currents + 3 phase voltages + 1 DC link
 *
 * Uses iLLD group DMA (IfxAdc_Tmadc_DmaConfig) for transfer.
 * Source address overridden to AW0.RES[] after initGroup — the iLLD
 * defaults to AW2 which has a different layout. AW0 is the plain
 * result window that matches what the debugger shows in RES0..RES6.
 *
 * DMA operationMode = single → HW request disabled after each transfer.
 * ISR re-arms it with IfxDma_enableChannelTransaction() so it fires
 * continuously every PWM cycle.
 */

#include "Adc_MotorSense.h"
#include "Dma/Std/IfxDma.h"
#include "IfxCpu.h"

/*═══════════════════════════════════════════════════════════════════════════
 * MODULE-LEVEL HANDLES
 *═══════════════════════════════════════════════════════════════════════════*/
static IfxAdc_Tmadc g_module;
static IfxAdc_Tmadc_Ch g_chIa, g_chIb, g_chIc;
static IfxAdc_Tmadc_Group g_grpCurr;
static IfxAdc_Tmadc_Ch g_chVa, g_chVb, g_chVc, g_chVdc;
static IfxAdc_Tmadc_Group g_grpVolt;

/*═══════════════════════════════════════════════════════════════════════════
 * DMA RESULT BUFFERS
 * 64-byte aligned — required by TC4Dx DMA move engine.
 *
 * Layout after DMA transfer (AW0 window, bits[15:4] = 12-bit result):
 *   g_dmaCurr[0] = RES0 (Ia)
 *   g_dmaCurr[1] = RES1 (Ib)
 *   g_dmaCurr[2] = RES2 (Ic)
 *
 *   g_dmaVolt[0] = RES3 (Va)
 *   g_dmaVolt[1] = RES4 (Vb)
 *   g_dmaVolt[2] = RES5 (Vc)
 *   g_dmaVolt[3] = RES6 (Vdc)
 *═══════════════════════════════════════════════════════════════════════════*/
static uint32 __attribute__((aligned(64))) g_dmaCurr[ADC_DMA_CURR_BUF_SIZE];
static uint32 __attribute__((aligned(64))) g_dmaVolt[ADC_DMA_VOLT_BUF_SIZE];

/*═══════════════════════════════════════════════════════════════════════════
 * RESULTS  (written by ISRs, read by FOC task)
 *═══════════════════════════════════════════════════════════════════════════*/
static volatile Adc_PhaseCurrents g_currents;
static volatile Adc_PhaseVoltages g_voltages;

/*═══════════════════════════════════════════════════════════════════════════
 * IIR FILTER STATES
 *═══════════════════════════════════════════════════════════════════════════*/
static float32 g_filtIa = (float32) ADC_ZERO_CURRENT_RAW;
static float32 g_filtIb = (float32) ADC_ZERO_CURRENT_RAW;
static float32 g_filtIc = (float32) ADC_ZERO_CURRENT_RAW;
static float32 g_filtVa = 0.0f;
static float32 g_filtVb = 0.0f;
static float32 g_filtVc = 0.0f;
static float32 g_filtVdc = 0.0f;

static volatile uint16 g_rawCurr[3];
static volatile uint16 g_rawVolt[4];
static volatile boolean g_currReady = FALSE;
static volatile boolean g_voltReady = FALSE;

/*═══════════════════════════════════════════════════════════════════════════
 * PRIVATE: MODULE INIT
 *═══════════════════════════════════════════════════════════════════════════*/
static void prv_initModule (void)
{
    IfxAdc_Tmadc_Config modCfg;
    IfxAdc_enableModule(&MODULE_ADC);
    IfxAdc_Tmadc_initModuleConfig(&modCfg, &MODULE_ADC);

    modCfg.id = ADC_TMADC_MODULE;
    modCfg.calEnable = TRUE;

    /* SR0 → DMA ch1 (current group) */
    IfxAdc_Tmadc_InterruptConfig sr0 = {.node = IfxAdc_TmadcServReq_0, .eventOp = IfxAdc_TmadcEventOp_orLogicWithoutWfc,
            .eventSel = IfxAdc_TmadcEventSel_result, .typeOfService = IfxSrc_Tos_dma0, .vmId = 0U, .priority =
                    (Ifx_Priority) ADC_DMA_CURR_CHANNEL};
    /* SR1 → DMA ch2 (voltage group) */
    IfxAdc_Tmadc_InterruptConfig sr1 = {.node = IfxAdc_TmadcServReq_1, .eventOp = IfxAdc_TmadcEventOp_orLogicWithoutWfc,
            .eventSel = IfxAdc_TmadcEventSel_result, .typeOfService = IfxSrc_Tos_dma0, .vmId = 0U, .priority =
                    (Ifx_Priority) ADC_DMA_VOLT_CHANNEL};

    IfxAdc_Tmadc_ServRequestConfig srvCfg;
    srvCfg.intConfig[0] = &sr0;
    srvCfg.intConfig[1] = &sr1;
    srvCfg.numServReqNodes = 2U;
    modCfg.srvReqCfg = &srvCfg;

    IfxAdc_Tmadc_initModule(&g_module, &modCfg);
}

/*═══════════════════════════════════════════════════════════════════════════
 * PRIVATE: CURRENT GROUP  (Ia, Ib, Ic)
 *═══════════════════════════════════════════════════════════════════════════*/
static void prv_initCurrentGroup (void)
{
    IfxAdc_Tmadc_ChannelTriggerConfig trig = {.hwTrigger1 = {ADC_TMADC_TRIG_MUXSEL, IfxAdc_TmadcTriggerMode_risingEdge},
            .hwTrigger2 = {IfxAdc_TmadcTriggerMuxSel_0, IfxAdc_TmadcTriggerMode_disable}, .delayNS =
            ADC_CURR_TRIGGER_DELAY_NS};

    IfxAdc_Tmadc_ChannelServReqConfig srvReq = {.errorNode = IfxAdc_TmadcServReq_none, .boundaryNode =
            IfxAdc_TmadcServReq_none, .resultNode = IfxAdc_TmadcServReq_0};

    /* DMA fires CPU interrupt at IFX_INTPRIO_ADC_CURR when transfer completes */
    IfxAdc_Tmadc_DmaSrvReq currDmaSrvReq = {.typeOfService = IfxSrc_Tos_cpu0, .vmId = 0U, .priority =
            (Ifx_Priority) IFX_INTPRIO_ADC_CURR};
    IfxAdc_Tmadc_DmaConfig currDmaCfg = {.channelId = ADC_DMA_CURR_CHANNEL, .dmaId = ADC_DMA_MODULE, .dmaSrvReqCfg =
            &currDmaSrvReq};

    IfxAdc_Tmadc_GroupConfig grpCfg;
    IfxAdc_Tmadc_initGroupConfig(&grpCfg);
    grpCfg.channels = (uint16) ((1U << ADC_CH_IA) | (1U << ADC_CH_IB) | (1U << ADC_CH_IC));
    grpCfg.numChannels = 3U;
    grpCfg.baseResultReg = ADC_RESREG_IA;
    grpCfg.trigger = &trig;
    grpCfg.grpSrvReq = &srvReq;
    grpCfg.mode = IfxAdc_TmadcOpMode_oneShot;
    grpCfg.waitForRead = FALSE;
    grpCfg.enableTimestamp = FALSE;
    grpCfg.moduleId = ADC_TMADC_MODULE;
    grpCfg.dmaCfg = &currDmaCfg;
    grpCfg.groupResPtr = (void*) g_dmaCurr;
    grpCfg.groupCallback = NULL_PTR;

    IfxAdc_Tmadc_ChConfig cfgIa;
    IfxAdc_Tmadc_initChannelConfig(&cfgIa, &MODULE_ADC);
    cfgIa.id = ADC_CH_IA;
    cfgIa.moduleId = ADC_TMADC_MODULE;
    cfgIa.samplingTimeNS = ADC_CURR_SAMPLING_TIME_NS;
    cfgIa.core = ADC_CORE_IA;
    cfgIa.mode = IfxAdc_TmadcOpMode_oneShot;
    cfgIa.resultCfg.resultReg = ADC_RESREG_IA;
    cfgIa.resultCfg.waitForRead = FALSE;
    cfgIa.resultCfg.enableTimestamp = FALSE;
    cfgIa.resultCfg.boundMode = FALSE;
    cfgIa.groupCfg = &grpCfg;

    IfxAdc_Tmadc_ChConfig cfgIb;
    IfxAdc_Tmadc_initChannelConfig(&cfgIb, &MODULE_ADC);
    cfgIb.id = ADC_CH_IB;
    cfgIb.moduleId = ADC_TMADC_MODULE;
    cfgIb.samplingTimeNS = ADC_CURR_SAMPLING_TIME_NS;
    cfgIb.core = ADC_CORE_IB;
    cfgIb.mode = IfxAdc_TmadcOpMode_oneShot;
    cfgIb.resultCfg.resultReg = ADC_RESREG_IB;
    cfgIb.resultCfg.waitForRead = FALSE;
    cfgIb.resultCfg.enableTimestamp = FALSE;
    cfgIb.resultCfg.boundMode = FALSE;
    cfgIb.groupCfg = &grpCfg;

    IfxAdc_Tmadc_ChConfig cfgIc;
    IfxAdc_Tmadc_initChannelConfig(&cfgIc, &MODULE_ADC);
    cfgIc.id = ADC_CH_IC;
    cfgIc.moduleId = ADC_TMADC_MODULE;
    cfgIc.samplingTimeNS = ADC_CURR_SAMPLING_TIME_NS;
    cfgIc.core = ADC_CORE_IC;
    cfgIc.mode = IfxAdc_TmadcOpMode_oneShot;
    cfgIc.resultCfg.resultReg = ADC_RESREG_IC;
    cfgIc.resultCfg.waitForRead = FALSE;
    cfgIc.resultCfg.enableTimestamp = FALSE;
    cfgIc.resultCfg.boundMode = FALSE;
    cfgIc.groupCfg = &grpCfg;

    IfxAdc_Tmadc_initChannel(&g_chIa, &cfgIa);
    IfxAdc_Tmadc_initChannel(&g_chIb, &cfgIb);
    IfxAdc_Tmadc_initChannel(&g_chIc, &cfgIc);
    IfxAdc_Tmadc_initGroup(&g_grpCurr, &grpCfg);

    /* DMA fires ISR at correct time; results read directly via readResult() in ISR */
}

/*═══════════════════════════════════════════════════════════════════════════
 * PRIVATE: VOLTAGE GROUP  (Va, Vb, Vc, Vdc)
 *═══════════════════════════════════════════════════════════════════════════*/
static void prv_initVoltageGroup (void)
{
    IfxAdc_Tmadc_ChannelTriggerConfig trig = {.hwTrigger1 = {ADC_TMADC_TRIG_MUXSEL, IfxAdc_TmadcTriggerMode_risingEdge},
            .hwTrigger2 = {IfxAdc_TmadcTriggerMuxSel_0, IfxAdc_TmadcTriggerMode_disable}, .delayNS =
            ADC_VOLT_TRIGGER_DELAY_NS};

    IfxAdc_Tmadc_ChannelServReqConfig srvReq = {.errorNode = IfxAdc_TmadcServReq_none, .boundaryNode =
            IfxAdc_TmadcServReq_none, .resultNode = IfxAdc_TmadcServReq_1};

    IfxAdc_Tmadc_DmaSrvReq voltDmaSrvReq = {.typeOfService = IfxSrc_Tos_cpu0, .vmId = 0U, .priority =
            (Ifx_Priority) IFX_INTPRIO_ADC_VOLT};
    IfxAdc_Tmadc_DmaConfig voltDmaCfg = {.channelId = ADC_DMA_VOLT_CHANNEL, .dmaId = ADC_DMA_MODULE, .dmaSrvReqCfg =
            &voltDmaSrvReq};

    IfxAdc_Tmadc_GroupConfig grpCfg;
    IfxAdc_Tmadc_initGroupConfig(&grpCfg);
    grpCfg.channels = (uint16) ((1U << ADC_CH_VA) | (1U << ADC_CH_VB) | (1U << ADC_CH_VC) | (1U << ADC_CH_VDC));
    grpCfg.numChannels = 4U;
    grpCfg.baseResultReg = ADC_RESREG_VA;
    grpCfg.trigger = &trig;
    grpCfg.grpSrvReq = &srvReq;
    grpCfg.mode = IfxAdc_TmadcOpMode_oneShot;
    grpCfg.waitForRead = FALSE;
    grpCfg.enableTimestamp = FALSE;
    grpCfg.moduleId = ADC_TMADC_MODULE;
    grpCfg.dmaCfg = &voltDmaCfg;
    grpCfg.groupResPtr = (void*) g_dmaVolt;
    grpCfg.groupCallback = NULL_PTR;

    IfxAdc_Tmadc_ChConfig cfgVa;
    IfxAdc_Tmadc_initChannelConfig(&cfgVa, &MODULE_ADC);
    cfgVa.id = ADC_CH_VA;
    cfgVa.moduleId = ADC_TMADC_MODULE;
    cfgVa.samplingTimeNS = ADC_VOLT_SAMPLING_TIME_NS;
    cfgVa.core = ADC_CORE_VA;
    cfgVa.mode = IfxAdc_TmadcOpMode_oneShot;
    cfgVa.resultCfg.resultReg = ADC_RESREG_VA;
    cfgVa.resultCfg.waitForRead = FALSE;
    cfgVa.resultCfg.boundMode = FALSE;
    cfgVa.groupCfg = &grpCfg;

    IfxAdc_Tmadc_ChConfig cfgVb;
    IfxAdc_Tmadc_initChannelConfig(&cfgVb, &MODULE_ADC);
    cfgVb.id = ADC_CH_VB;
    cfgVb.moduleId = ADC_TMADC_MODULE;
    cfgVb.samplingTimeNS = ADC_VOLT_SAMPLING_TIME_NS;
    cfgVb.core = ADC_CORE_VB;
    cfgVb.mode = IfxAdc_TmadcOpMode_oneShot;
    cfgVb.resultCfg.resultReg = ADC_RESREG_VB;
    cfgVb.resultCfg.waitForRead = FALSE;
    cfgVb.resultCfg.boundMode = FALSE;
    cfgVb.groupCfg = &grpCfg;

    IfxAdc_Tmadc_ChConfig cfgVc;
    IfxAdc_Tmadc_initChannelConfig(&cfgVc, &MODULE_ADC);
    cfgVc.id = ADC_CH_VC;
    cfgVc.moduleId = ADC_TMADC_MODULE;
    cfgVc.samplingTimeNS = ADC_VOLT_SAMPLING_TIME_NS;
    cfgVc.core = ADC_CORE_VC;
    cfgVc.mode = IfxAdc_TmadcOpMode_oneShot;
    cfgVc.resultCfg.resultReg = ADC_RESREG_VC;
    cfgVc.resultCfg.waitForRead = FALSE;
    cfgVc.resultCfg.boundMode = FALSE;
    cfgVc.groupCfg = &grpCfg;

    IfxAdc_Tmadc_ChConfig cfgVdc;
    IfxAdc_Tmadc_initChannelConfig(&cfgVdc, &MODULE_ADC);
    cfgVdc.id = ADC_CH_VDC;
    cfgVdc.moduleId = ADC_TMADC_MODULE;
    cfgVdc.samplingTimeNS = ADC_VOLT_SAMPLING_TIME_NS;
    cfgVdc.core = ADC_CORE_VDC;
    cfgVdc.mode = IfxAdc_TmadcOpMode_oneShot;
    cfgVdc.resultCfg.resultReg = ADC_RESREG_VDC;
    cfgVdc.resultCfg.waitForRead = FALSE;
    cfgVdc.resultCfg.boundMode = FALSE;
    cfgVdc.groupCfg = &grpCfg;

    IfxAdc_Tmadc_initChannel(&g_chVa, &cfgVa);
    IfxAdc_Tmadc_initChannel(&g_chVb, &cfgVb);
    IfxAdc_Tmadc_initChannel(&g_chVc, &cfgVc);
    IfxAdc_Tmadc_initChannel(&g_chVdc, &cfgVdc);
    IfxAdc_Tmadc_initGroup(&g_grpVolt, &grpCfg);

    /* DMA fires ISR at correct time; results read directly via readResult() in ISR */
}

/*═══════════════════════════════════════════════════════════════════════════
 * PRIVATE: EGTM TRIGGER
 *═══════════════════════════════════════════════════════════════════════════*/
static void prv_connectEgtmTrigger (void)
{
    boolean ok = IfxEgtm_Trigger_trigToAdc(
    ADC_EGTM_CLUSTER,
    ADC_EGTM_TRIG_SOURCE,
    ADC_EGTM_TRIG_CHANNEL,
    ADC_EGTM_TRIG_SIGNAL);
    IFX_ASSERT(IFX_VERBOSE_LEVEL_ERROR, ok == TRUE);
}

/*═══════════════════════════════════════════════════════════════════════════
 * PUBLIC API
 *═══════════════════════════════════════════════════════════════════════════*/

void Adc_MotorSense_init (void)
{
    prv_initModule();
    prv_initCurrentGroup();
    prv_initVoltageGroup();
    prv_connectEgtmTrigger();

    g_filtIa = (float32) ADC_ZERO_CURRENT_RAW;
    g_filtIb = (float32) ADC_ZERO_CURRENT_RAW;
    g_filtIc = (float32) ADC_ZERO_CURRENT_RAW;
    g_filtVa = 0.0f;
    g_filtVb = 0.0f;
    g_filtVc = 0.0f;
    g_filtVdc = 0.0f;
}

void Adc_MotorSense_start (void)
{
    IfxAdc_Tmadc_runModule(&g_module);
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

/*═══════════════════════════════════════════════════════════════════════════
 * CURRENT ISR  (~1.2 us after PWM valley)
 *
 * The DMA fires this ISR at the right time (after all 3 currents done).
 * Results are read directly via IfxAdc_Tmadc_readChannelResult() — this uses
 * rsltSFR which the iLLD computed correctly from AW1, avoiding any
 * address-window ambiguity. Range: 0..4095 (12-bit).
 *═══════════════════════════════════════════════════════════════════════════*/
void Adc_MotorSense_voltIsr(void)
{
    /* Read results FIRST, before re-arming DMA */
    g_rawVolt[0] = IfxAdc_Tmadc_readChannelResult(&g_chVa);
    g_rawVolt[1] = IfxAdc_Tmadc_readChannelResult(&g_chVb);
    g_rawVolt[2] = IfxAdc_Tmadc_readChannelResult(&g_chVc);
    g_rawVolt[3] = IfxAdc_Tmadc_readChannelResult(&g_chVdc);
    g_voltReady  = TRUE;

    /* Re-arm DMA AFTER reading — no bus contention */
    IfxDma_setChannelDestinationAddress(&MODULE_DMA0, ADC_DMA_VOLT_CHANNEL, (uint32)g_dmaVolt);
    IfxDma_setChannelSourceAddress(&MODULE_DMA0, ADC_DMA_VOLT_CHANNEL, (uint32)g_grpVolt.sourceAddress);
    IfxDma_enableChannelTransaction(&MODULE_DMA0, ADC_DMA_VOLT_CHANNEL);
}


void Adc_MotorSense_currIsr(void)
{
    /* Read results FIRST */
    g_rawCurr[0] = IfxAdc_Tmadc_readChannelResult(&g_chIa);
    g_rawCurr[1] = IfxAdc_Tmadc_readChannelResult(&g_chIb);
    g_rawCurr[2] = IfxAdc_Tmadc_readChannelResult(&g_chIc);
    g_currReady  = TRUE;

    /* Re-arm DMA AFTER reading */
    IfxDma_setChannelDestinationAddress(&MODULE_DMA0, ADC_DMA_CURR_CHANNEL, (uint32)g_dmaCurr);
    IfxDma_setChannelSourceAddress(&MODULE_DMA0, ADC_DMA_CURR_CHANNEL, (uint32)g_grpCurr.sourceAddress);
    IfxDma_enableChannelTransaction(&MODULE_DMA0, ADC_DMA_CURR_CHANNEL);
}
