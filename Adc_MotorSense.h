#ifndef ADC_MOTOR_SENSE_H
#define ADC_MOTOR_SENSE_H

#include "Ifx_Types.h"
#include "Adc/Tmadc/IfxAdc_Tmadc.h"
#include "Egtm/Trigger/IfxEgtm_Trigger.h"
#include "Dma/Dma/IfxDma_Dma.h"
#include "EGTM_ATOM_3_Phase_Inverter_PWM.h"

/*─── TMADC MODULE ───*/
#define ADC_TMADC_MODULE            IfxAdc_TmadcModule_0

/*─── CURRENT CHANNELS ───*/
#define ADC_CH_IA                   IfxAdc_TmadcChannel_0
#define ADC_CH_IB                   IfxAdc_TmadcChannel_1
#define ADC_CH_IC                   IfxAdc_TmadcChannel_2
#define ADC_CORE_IA                 IfxAdc_TmadcSarCore_0
#define ADC_CORE_IB                 IfxAdc_TmadcSarCore_1
#define ADC_CORE_IC                 IfxAdc_TmadcSarCore_0
#define ADC_RESREG_IA               IfxAdc_TmadcResultReg_0
#define ADC_RESREG_IB               IfxAdc_TmadcResultReg_1
#define ADC_RESREG_IC               IfxAdc_TmadcResultReg_2

/*─── VOLTAGE CHANNELS ───*/
#define ADC_CH_VA                   IfxAdc_TmadcChannel_3
#define ADC_CH_VB                   IfxAdc_TmadcChannel_4
#define ADC_CH_VC                   IfxAdc_TmadcChannel_5
#define ADC_CH_VDC                  IfxAdc_TmadcChannel_6
#define ADC_CORE_VA                 IfxAdc_TmadcSarCore_0
#define ADC_CORE_VB                 IfxAdc_TmadcSarCore_1
#define ADC_CORE_VC                 IfxAdc_TmadcSarCore_0
#define ADC_CORE_VDC                IfxAdc_TmadcSarCore_1
#define ADC_RESREG_VA               IfxAdc_TmadcResultReg_3
#define ADC_RESREG_VB               IfxAdc_TmadcResultReg_4
#define ADC_RESREG_VC               IfxAdc_TmadcResultReg_5
#define ADC_RESREG_VDC              IfxAdc_TmadcResultReg_6

/*─── EGTM TRIGGER: Ch0 output → ADC_OUT crossbar → TMADC HWTR ───*/
#define ADC_EGTM_CLUSTER            IfxEgtm_Cluster_0
#define ADC_EGTM_TRIG_SOURCE        IfxEgtm_TrigSource_atom
#define ADC_EGTM_TRIG_CHANNEL       IfxEgtm_TrigChannel_3
#define ADC_EGTM_TRIG_SIGNAL        IfxEgtm_Cfg_AdcTriggerSignal_0
#define ADC_TMADC_TRIG_MUXSEL       IfxAdc_TmadcTriggerMuxSel_36

/*─── TRIGGER DELAY ───
 * based on the current duty, so the ADC always samples at the peak.
 */
#define ADC_CURR_TRIGGER_DELAY_NS   0.0f
#define ADC_VOLT_TRIGGER_DELAY_NS   2000.0f

/*─── SAMPLING TIMES ───*/
#define ADC_CURR_SAMPLING_TIME_NS   (100.0f)
#define ADC_VOLT_SAMPLING_TIME_NS   (100.0f)

/*─── DMA ───*/
#define ADC_DMA_CURR_CHANNEL        IfxDma_ChannelId_1
#define ADC_DMA_VOLT_CHANNEL        IfxDma_ChannelId_2
#define ADC_DMA_MODULE              IfxDma_Index_0
#define ADC_DMA_CURR_BUF_SIZE       (3U)
#define ADC_DMA_VOLT_BUF_SIZE       (4U)

/*─── ISR PRIORITIES ───*/
#define IFX_INTPRIO_ADC_CURR        50U
#define IFX_INTPRIO_ADC_VOLT        48U

/*─── SCALING ───*/
#define ADC_CURRENT_FULL_SCALE_A    (50.0f)
#define ADC_ZERO_CURRENT_RAW        (2048U)
#define ADC_VDC_FULL_SCALE_V        (800.0f)
#define ADC_VPHASE_FULL_SCALE_V     (800.0f)

#define ADC_RAW_TO_AMPS(raw) \
    (((float32)((raw) >> 4U) - (float32)ADC_ZERO_CURRENT_RAW) * \
     (ADC_CURRENT_FULL_SCALE_A / (float32)ADC_ZERO_CURRENT_RAW))

#define ADC_RAW_TO_VOLTS_PHASE(raw) \
    ((float32)((raw) >> 4U) * (ADC_VPHASE_FULL_SCALE_V / 4095.0f))

#define ADC_RAW_TO_VOLTS_DC(raw) \
    ((float32)((raw) >> 4U) * (ADC_VDC_FULL_SCALE_V / 4095.0f))

/*─── FILTER ───*/
#define ADC_FILTER_ALPHA_CURR       (0.8f)
#define ADC_FILTER_ALPHA_VPHASE     (0.3f)
#define ADC_FILTER_ALPHA_VDC        (0.1f)

/*─── DATA TYPES ───*/
typedef struct
{
    float32 ia, ib, ic;
    float32 kirchhoff_err;
    uint32 timestamp;
} Adc_PhaseCurrents;

typedef struct
{
    float32 va, vb, vc, vdc;
} Adc_PhaseVoltages;

/*─── API ───*/
void Adc_MotorSense_init (void);
void Adc_MotorSense_start (void);

const volatile uint16* Adc_MotorSense_getRawCurrents(void);

const Adc_PhaseCurrents* Adc_MotorSense_getCurrents (void);
const Adc_PhaseVoltages* Adc_MotorSense_getVoltages (void);
void Adc_MotorSense_currIsr (void);
void Adc_MotorSense_voltIsr (void);

#endif
