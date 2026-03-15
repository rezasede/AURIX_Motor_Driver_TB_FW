#include "IfxAdc_Tmadc.h"
#include "IfxDma_Dma.h"
#include "IfxSrc.h"
#include "IfxCpu_Irq.h"

/* ===================== Configuration ===================== */

#define DMA_CHANNEL                 IfxDma_ChannelId_0
#define DMA_ISR_PRIORITY            50
#define NUM_LINKED_LIST_ITEMS       2

#define TMADC_SERVICE_NODE          IfxAdc_TmadcServReq_0

#define TMADC_CH0_INPUT             IfxAdc_TmadcChannel_0
#define TMADC_CH1_INPUT             IfxAdc_TmadcChannel_1
#define TMADC_SAMPLING_TIME_NS      200

/* ===================== Global Variables ===================== */

IfxAdc_Tmadc        g_tmadc;
IfxAdc_Tmadc_Ch     g_adcCh0;
IfxAdc_Tmadc_Ch     g_adcCh1;

IfxDma_Dma          g_dma;
IfxDma_Dma_Channel  g_dmaChannel;

Ifx_DMA_CH IFX_ALIGN(256) g_linkedList[NUM_LINKED_LIST_ITEMS];

volatile uint16 g_phaseU;
volatile uint16 g_phaseV;
volatile boolean g_newDataReady = FALSE;

/* ===================== DMA Interrupt ===================== */

IFX_INTERRUPT(dmaISR, 0, DMA_ISR_PRIORITY);

void dmaISR(void)
{
    g_newDataReady = TRUE;
    IfxCpu_enableInterrupts();
}

/* ===================== DMA Init ===================== */

static void initDMA(void)
{
    IfxDma_Dma_Config dmaConfig;
    IfxDma_Dma_initModuleConfig(&dmaConfig, &MODULE_DMA0);
    IfxDma_Dma_initModule(&g_dma, &dmaConfig);

    IfxDma_Dma_ChannelConfig cfg;
    IfxDma_Dma_initChannelConfig(&cfg, &g_dma);

    cfg.channelId                = DMA_CHANNEL;
    cfg.transferCount            = 1;
    cfg.moveSize                 = IfxDma_ChannelMoveSize_16bit;
    cfg.requestMode              = IfxDma_ChannelRequestMode_completeTransactionPerRequest;
    cfg.shadowControl            = IfxDma_ChannelShadow_linkedList;
    cfg.hardwareRequestEnabled   = TRUE;

    /*
     * CRITICAL: Must be continuous so the hardware request enable (HRE)
     * bit stays set after each transaction. The default is "single" which
     * clears HRE after one transaction — DMA goes deaf to all further
     * triggers from TMADC.
     */
    cfg.operationMode            = IfxDma_ChannelOperationMode_continuous;

    /* ---- TCS0: read result 0 -> g_phaseU ---- */
    cfg.sourceAddress            = (uint32)&MODULE_ADC.TMADC[0].AW0.RES[0];
    cfg.destinationAddress       = (uint32)&g_phaseU;
    cfg.shadowAddress            = (uint32)&g_linkedList[1];
    cfg.channelInterruptEnabled  = FALSE;

    IfxDma_Dma_initLinkedListEntry(&g_linkedList[0], &cfg);
    g_linkedList[0].CHCSR.B.SCH  = 0;

    /* ---- TCS1: read result 1 -> g_phaseV ---- */
    cfg.sourceAddress            = (uint32)&MODULE_ADC.TMADC[0].AW0.RES[1];
    cfg.destinationAddress       = (uint32)&g_phaseV;
    cfg.shadowAddress            = (uint32)&g_linkedList[0];
    cfg.channelInterruptEnabled  = TRUE;

    IfxDma_Dma_initLinkedListEntry(&g_linkedList[1], &cfg);
    g_linkedList[1].CHCSR.B.SCH  = 1;

    /* ---- Load TCS0 into the DMA channel ---- */
    cfg.sourceAddress            = (uint32)&MODULE_ADC.TMADC[0].AW0.RES[0];
    cfg.destinationAddress       = (uint32)&g_phaseU;
    cfg.shadowAddress            = (uint32)&g_linkedList[1];
    cfg.channelInterruptEnabled  = FALSE;

    IfxDma_Dma_initChannel(&g_dmaChannel, &cfg);

    volatile Ifx_SRC_SRCR *src = IfxDma_Dma_getSrcPointer(&g_dmaChannel);
    IfxSrc_init(src, IfxSrc_Tos_cpu0, DMA_ISR_PRIORITY, IfxSrc_VmId_0);
    IfxSrc_enable(src);

    MODULE_DMA0.TSR[DMA_CHANNEL].B.ECH = 1;
}

/* ===================== TMADC Init ===================== */

void initTMADC(void)
{
    /* 1. Enable ADC module */
    IfxAdc_enableModule(&MODULE_ADC);

    /* 2. Init DMA before conversions start */
    initDMA();

    /* 3. TMADC module config */
    IfxAdc_Tmadc_Config tmadcCfg;
    IfxAdc_Tmadc_initModuleConfig(&tmadcCfg, &MODULE_ADC);

    tmadcCfg.id        = IfxAdc_TmadcModule_0;
    tmadcCfg.calEnable = 1;

    /* Service request: OR without WFC -> fires on each result event */
    IfxAdc_Tmadc_InterruptConfig intCfg;

    intCfg.node          = TMADC_SERVICE_NODE;
    intCfg.eventSel      = IfxAdc_TmadcEventSel_result;
    intCfg.eventOp       = IfxAdc_TmadcEventOp_orLogicWithoutWfc;
    intCfg.typeOfService  = IfxSrc_Tos_dma0;
    intCfg.vmId          = IfxSrc_VmId_0;
    intCfg.priority      = DMA_CHANNEL;

    IfxAdc_Tmadc_ServRequestConfig srvCfg;
    srvCfg.intConfig[0]     = &intCfg;
    srvCfg.numServReqNodes  = 1;

    tmadcCfg.srvReqCfg = &srvCfg;

    IfxAdc_Tmadc_initModule(&g_tmadc, &tmadcCfg);

    /* 4. Channel 0 */
    IfxAdc_Tmadc_ChConfig chCfg;
    IfxAdc_Tmadc_initChannelConfig(&chCfg, &MODULE_ADC);

    chCfg.id                      = TMADC_CH0_INPUT;
    chCfg.moduleId                = IfxAdc_TmadcModule_0;
    chCfg.mode                    = IfxAdc_TmadcOpMode_continuous;
    chCfg.core                    = IfxAdc_TmadcSarCore_0;
    chCfg.samplingTimeNS          = TMADC_SAMPLING_TIME_NS;

    chCfg.resultCfg.resultReg     = IfxAdc_TmadcResultReg_0;
    chCfg.resultCfg.waitForRead   = 1;

    IfxAdc_Tmadc_ChannelServReqConfig chSrv0;
    chSrv0.resultNode    = TMADC_SERVICE_NODE;
    chSrv0.errorNode     = IfxAdc_TmadcServReq_none;
    chSrv0.boundaryNode  = IfxAdc_TmadcServReq_none;

    chCfg.channelSrvReq = &chSrv0;

    IfxAdc_Tmadc_initChannel(&g_adcCh0, &chCfg);

    /* 5. Channel 1 */
    IfxAdc_Tmadc_initChannelConfig(&chCfg, &MODULE_ADC);

    chCfg.id                      = TMADC_CH1_INPUT;
    chCfg.moduleId                = IfxAdc_TmadcModule_0;
    chCfg.mode                    = IfxAdc_TmadcOpMode_continuous;
    chCfg.core                    = IfxAdc_TmadcSarCore_0;
    chCfg.samplingTimeNS          = TMADC_SAMPLING_TIME_NS;

    chCfg.resultCfg.resultReg     = IfxAdc_TmadcResultReg_1;
    chCfg.resultCfg.waitForRead   = 1;

    IfxAdc_Tmadc_ChannelServReqConfig chSrv1;
    chSrv1.resultNode    = TMADC_SERVICE_NODE;
    chSrv1.errorNode     = IfxAdc_TmadcServReq_none;
    chSrv1.boundaryNode  = IfxAdc_TmadcServReq_none;

    chCfg.channelSrvReq = &chSrv1;

    IfxAdc_Tmadc_initChannel(&g_adcCh1, &chCfg);

    /* 6. Start conversions */
    IfxAdc_Tmadc_runModule(&g_tmadc);
}
