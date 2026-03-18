/**
 * @file    Adc_MotorSense.c
 * @brief   7-channel TMADC + DMA motor sensing
 *
 * Trigger: Ch0 PWM output rising edge → TMADC HW trigger → HWTRDLYEN delay
 * The delay (TRD) is dynamically updated by the ATOM period ISR every cycle
 * so the ADC always samples at the valley regardless of duty cycle.
 *
 * DMA operationMode = single → ISR re-arms with reset DADR each cycle.
 */

#include "Adc_MotorSense.h"
#include "Dma/Std/IfxDma.h"
#include "IfxCpu.h"

/*═══════════════════════════════════════════════════════════════════════════*/
static IfxAdc_Tmadc g_module;
static IfxAdc_Tmadc_Ch g_chIa, g_chIb, g_chIc;
static IfxAdc_Tmadc_Group g_grpCurr;
static IfxAdc_Tmadc_Ch g_chVa, g_chVb, g_chVc, g_chVdc;
static IfxAdc_Tmadc_Group g_grpVolt;

static uint32 __attribute__((aligned(64))) g_dmaCurr[ADC_DMA_CURR_BUF_SIZE];
static uint32 __attribute__((aligned(64))) g_dmaVolt[ADC_DMA_VOLT_BUF_SIZE];

volatile Adc_PhaseCurrents g_currents;
volatile Adc_PhaseVoltages g_voltages;
volatile uint16 g_rawCurr[3];
volatile uint16 g_rawVolt[4];
volatile boolean g_currReady = FALSE;
volatile boolean g_voltReady = FALSE;

/*═══════════════════════════════════════════════════════════════════════════
 * MODULE INIT
 *═══════════════════════════════════════════════════════════════════════════*/
static void prv_initModule (void)
{
    IfxAdc_Tmadc_Config modCfg;
    IfxAdc_enableModule(&MODULE_ADC);
    IfxAdc_Tmadc_initModuleConfig(&modCfg, &MODULE_ADC);

    modCfg.id = ADC_TMADC_MODULE;
    modCfg.calEnable = TRUE;

    IfxAdc_Tmadc_InterruptConfig sr0 = {.node = IfxAdc_TmadcServReq_0, .eventOp = IfxAdc_TmadcEventOp_orLogicWithoutWfc,
            .eventSel = IfxAdc_TmadcEventSel_result, .typeOfService = IfxSrc_Tos_dma0, .vmId = 0U, .priority =
                    (Ifx_Priority) ADC_DMA_CURR_CHANNEL};
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
 * CURRENT GROUP
 *═══════════════════════════════════════════════════════════════════════════*/
static void prv_initCurrentGroup (void)
{
    IfxAdc_Tmadc_ChannelTriggerConfig trig = {.hwTrigger1 = {ADC_TMADC_TRIG_MUXSEL, IfxAdc_TmadcTriggerMode_risingEdge},
            .hwTrigger2 = {IfxAdc_TmadcTriggerMuxSel_0, IfxAdc_TmadcTriggerMode_disable}, .delayNS =
            ADC_CURR_TRIGGER_DELAY_NS};
    IfxAdc_Tmadc_ChannelServReqConfig srvReq = {.errorNode = IfxAdc_TmadcServReq_none, .boundaryNode =
            IfxAdc_TmadcServReq_none, .resultNode = IfxAdc_TmadcServReq_0};
    IfxAdc_Tmadc_DmaSrvReq dmaSrvReq = {.typeOfService = IfxSrc_Tos_cpu0, .vmId = 0U, .priority =
            (Ifx_Priority) IFX_INTPRIO_ADC_CURR};
    IfxAdc_Tmadc_DmaConfig dmaCfg = {.channelId = ADC_DMA_CURR_CHANNEL, .dmaId = ADC_DMA_MODULE, .dmaSrvReqCfg =
            &dmaSrvReq};

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
    grpCfg.dmaCfg = &dmaCfg;
    grpCfg.groupResPtr = (void*) g_dmaCurr;
    grpCfg.groupCallback = NULL_PTR;

    IfxAdc_Tmadc_ChConfig cfg;

    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_IA;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.samplingTimeNS = ADC_CURR_SAMPLING_TIME_NS;
    cfg.core = ADC_CORE_IA;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.resultCfg.resultReg = ADC_RESREG_IA;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chIa, &cfg);

    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_IB;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.samplingTimeNS = ADC_CURR_SAMPLING_TIME_NS;
    cfg.core = ADC_CORE_IB;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.resultCfg.resultReg = ADC_RESREG_IB;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chIb, &cfg);

    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_IC;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.samplingTimeNS = ADC_CURR_SAMPLING_TIME_NS;
    cfg.core = ADC_CORE_IC;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.resultCfg.resultReg = ADC_RESREG_IC;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.enableTimestamp = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chIc, &cfg);

    IfxAdc_Tmadc_initGroup(&g_grpCurr, &grpCfg);
}

/*═══════════════════════════════════════════════════════════════════════════
 * VOLTAGE GROUP
 *═══════════════════════════════════════════════════════════════════════════*/
static void prv_initVoltageGroup (void)
{
    IfxAdc_Tmadc_ChannelTriggerConfig trig = {.hwTrigger1 = {ADC_TMADC_TRIG_MUXSEL, IfxAdc_TmadcTriggerMode_risingEdge},
            .hwTrigger2 = {IfxAdc_TmadcTriggerMuxSel_0, IfxAdc_TmadcTriggerMode_disable}, .delayNS =
            ADC_VOLT_TRIGGER_DELAY_NS};
    IfxAdc_Tmadc_ChannelServReqConfig srvReq = {.errorNode = IfxAdc_TmadcServReq_none, .boundaryNode =
            IfxAdc_TmadcServReq_none, .resultNode = IfxAdc_TmadcServReq_1};
    IfxAdc_Tmadc_DmaSrvReq dmaSrvReq = {.typeOfService = IfxSrc_Tos_cpu0, .vmId = 0U, .priority =
            (Ifx_Priority) IFX_INTPRIO_ADC_VOLT};
    IfxAdc_Tmadc_DmaConfig dmaCfg = {.channelId = ADC_DMA_VOLT_CHANNEL, .dmaId = ADC_DMA_MODULE, .dmaSrvReqCfg =
            &dmaSrvReq};

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
    grpCfg.dmaCfg = &dmaCfg;
    grpCfg.groupResPtr = (void*) g_dmaVolt;
    grpCfg.groupCallback = NULL_PTR;

    IfxAdc_Tmadc_ChConfig cfg;

    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_VA;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.samplingTimeNS = ADC_VOLT_SAMPLING_TIME_NS;
    cfg.core = ADC_CORE_VA;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.resultCfg.resultReg = ADC_RESREG_VA;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chVa, &cfg);

    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_VB;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.samplingTimeNS = ADC_VOLT_SAMPLING_TIME_NS;
    cfg.core = ADC_CORE_VB;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.resultCfg.resultReg = ADC_RESREG_VB;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chVb, &cfg);

    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_VC;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.samplingTimeNS = ADC_VOLT_SAMPLING_TIME_NS;
    cfg.core = ADC_CORE_VC;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.resultCfg.resultReg = ADC_RESREG_VC;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chVc, &cfg);

    IfxAdc_Tmadc_initChannelConfig(&cfg, &MODULE_ADC);
    cfg.id = ADC_CH_VDC;
    cfg.moduleId = ADC_TMADC_MODULE;
    cfg.samplingTimeNS = ADC_VOLT_SAMPLING_TIME_NS;
    cfg.core = ADC_CORE_VDC;
    cfg.mode = IfxAdc_TmadcOpMode_oneShot;
    cfg.resultCfg.resultReg = ADC_RESREG_VDC;
    cfg.resultCfg.waitForRead = FALSE;
    cfg.resultCfg.boundMode = FALSE;
    cfg.groupCfg = &grpCfg;
    IfxAdc_Tmadc_initChannel(&g_chVdc, &cfg);

    IfxAdc_Tmadc_initGroup(&g_grpVolt, &grpCfg);
}

/*═══════════════════════════════════════════════════════════════════════════
 * EGTM TRIGGER ROUTING
 *═══════════════════════════════════════════════════════════════════════════*/
static void prv_connectEgtmTrigger (void)
{
    boolean ok = IfxEgtm_Trigger_trigToAdc(
    ADC_EGTM_CLUSTER, ADC_EGTM_TRIG_SOURCE,
    ADC_EGTM_TRIG_CHANNEL, ADC_EGTM_TRIG_SIGNAL);
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

    MODULE_ADC.TMADC[0].CH[ADC_CH_IA].CFG.B.HWTRDLYEN = 0;
    MODULE_ADC.TMADC[0].CH[ADC_CH_IB].CFG.B.HWTRDLYEN = 0;
    MODULE_ADC.TMADC[0].CH[ADC_CH_IC].CFG.B.HWTRDLYEN = 0;
    MODULE_ADC.TMADC[0].CH[ADC_CH_VA].CFG.B.HWTRDLYEN = 1;
    MODULE_ADC.TMADC[0].CH[ADC_CH_VB].CFG.B.HWTRDLYEN = 1;
    MODULE_ADC.TMADC[0].CH[ADC_CH_VC].CFG.B.HWTRDLYEN = 1;
    MODULE_ADC.TMADC[0].CH[ADC_CH_VDC].CFG.B.HWTRDLYEN = 1;

}

void Adc_MotorSense_start (void)
{
    IfxAdc_Tmadc_runModule(&g_module);
    while (g_module.state != IfxAdc_Tmadc_moduleState_run)
    {
    }
}

const volatile uint16* Adc_MotorSense_getRawCurrents (void)
{
    return (const volatile uint16*) g_rawCurr;
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
 * VOLTAGE ISR
 *═══════════════════════════════════════════════════════════════════════════*/
void Adc_MotorSense_voltIsr (void)
{
    g_rawVolt[0] = IfxAdc_Tmadc_readChannelResult(&g_chVa);
    g_rawVolt[1] = IfxAdc_Tmadc_readChannelResult(&g_chVb);
    g_rawVolt[2] = IfxAdc_Tmadc_readChannelResult(&g_chVc);
    g_rawVolt[3] = IfxAdc_Tmadc_readChannelResult(&g_chVdc);
    g_voltReady = TRUE;

    IfxDma_setChannelDestinationAddress(&MODULE_DMA0, ADC_DMA_VOLT_CHANNEL, (uint32) g_dmaVolt);
    IfxDma_setChannelSourceAddress(&MODULE_DMA0, ADC_DMA_VOLT_CHANNEL, (uint32) g_grpVolt.sourceAddress);
    IfxDma_enableChannelTransaction(&MODULE_DMA0, ADC_DMA_VOLT_CHANNEL);
}

/*═══════════════════════════════════════════════════════════════════════════
 * CURRENT ISR
 *═══════════════════════════════════════════════════════════════════════════*/
void Adc_MotorSense_currIsr (void)
{
    g_rawCurr[0] = IfxAdc_Tmadc_readChannelResult(&g_chIa);
    g_rawCurr[1] = IfxAdc_Tmadc_readChannelResult(&g_chIb);
    g_rawCurr[2] = IfxAdc_Tmadc_readChannelResult(&g_chIc);
    g_currReady = TRUE;

    IfxDma_setChannelDestinationAddress(&MODULE_DMA0, ADC_DMA_CURR_CHANNEL, (uint32) g_dmaCurr);
    IfxDma_setChannelSourceAddress(&MODULE_DMA0, ADC_DMA_CURR_CHANNEL, (uint32) g_grpCurr.sourceAddress);
    IfxDma_enableChannelTransaction(&MODULE_DMA0, ADC_DMA_CURR_CHANNEL);
}
