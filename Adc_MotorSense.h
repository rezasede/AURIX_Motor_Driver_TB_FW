/**
 * @file    Adc_MotorSense.h
 * @brief   7-channel synchronized ADC for FOC on TC4D7XP
 *          3 phase currents + 3 phase voltages + 1 DC link voltage
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * ARCHITECTURE OVERVIEW
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *  Two TMADC groups, two DMA channels, two ISR priorities:
 *
 *  GROUP_CURR  (priority: HIGH)
 *  ─────────────────────────────────────────────────────────────────────────
 *  Trigger: EGTM valley (counter == 0, center-aligned PWM)
 *  Channels: Ia (SARcore0), Ib (SARcore1) → both convert in parallel
 *            Ic (SARcore0, after Ia finishes) → sequential on core0
 *
 *  Since the TC4Dx TMADC has only 2 SAR cores, 3 channels cannot all be
 *  simultaneous. Strategy: Ia and Ib share the two cores (truly parallel,
 *  ~600 ns). Ic is assigned to core0 — it starts as soon as Ia completes
 *  (~600 ns later). Total current group latency ≈ 1.2 µs. At 20 kHz PWM
 *  (50 µs period) this is 2.4% of the period — negligible ripple error on Ic.
 *  Result event fires when ALL 3 channels complete → SR0 → DMA ch1
 *
 *  GROUP_VOLT  (priority: LOW)
 *  ─────────────────────────────────────────────────────────────────────────
 *  Trigger: same EGTM valley but with +2000 ns delay
 *  Channels: Va (SARcore0), Vb (SARcore1) → parallel
 *            Vc (SARcore0), Vdc (SARcore1) → parallel, second round
 *
 *  Voltage signals change slowly compared to current (phase voltage is the
 *  PWM-filtered average, DC link is capacitor-stabilized). A 2 µs offset
 *  from the current group is irrelevant. 4 channels complete in ~1.2 µs.
 *  Result event fires when ALL 4 channels complete → SR1 → DMA ch2
 *
 *  TIMING DIAGRAM (one PWM period, 50 µs at 20 kHz):
 *
 *  t=0 µs   : EGTM valley → simultaneous triggers
 *  t=0      : GROUP_CURR trigger fires (delay=0)   → Ia+Ib convert (parallel)
 *  t=0.6 µs : Ia done, Ic starts on core0; Ib done
 *  t=1.2 µs : Ic done → SR0 fires → DMA ch1 copies 3 results → CPU ISR_CURR
 *  t=2.0 µs : GROUP_VOLT trigger fires (delay=2000ns) → Va+Vb convert (parallel)
 *  t=2.6 µs : Va done, Vc starts; Vb done, Vdc starts (both parallel)
 *  t=3.2 µs : Vc+Vdc done → SR1 fires → DMA ch2 copies 4 results → CPU ISR_VOLT
 *  t=50 µs  : next PWM valley → cycle repeats
 *
 *  ISR_CURR: filter, scale, Kirchhoff check, signal FOC task  ← TIME CRITICAL
 *  ISR_VOLT: filter, scale, update BEMF observer / DC bus     ← lower priority
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * USER CONFIGURATION — adapt to your board
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef ADC_MOTOR_SENSE_H
#define ADC_MOTOR_SENSE_H

#include "Ifx_Types.h"
#include "Adc/Tmadc/IfxAdc_Tmadc.h"
#include "Egtm/Trigger/IfxEgtm_Trigger.h"
#include "Dma/Dma/IfxDma_Dma.h"

/*─────────────────────────────────────────────────────────────────────────────
 * TMADC MODULE
 *───────────────────────────────────────────────────────────────────────────*/
#define ADC_TMADC_MODULE            IfxAdc_TmadcModule_0

/*─────────────────────────────────────────────────────────────────────────────
 * CURRENT CHANNELS  (verify against your PCB pin assignment)
 * All three must be on the same TMADC module.
 * Core assignment: Ia/Ic share SARcore0, Ib uses SARcore1.
 *───────────────────────────────────────────────────────────────────────────*/
#define ADC_CH_IA                   IfxAdc_TmadcChannel_0
#define ADC_CH_IB                   IfxAdc_TmadcChannel_1
#define ADC_CH_IC                   IfxAdc_TmadcChannel_2

#define ADC_CORE_IA                 IfxAdc_TmadcSarCore_0   /* parallel with Ib */
#define ADC_CORE_IB                 IfxAdc_TmadcSarCore_1   /* parallel with Ia */
#define ADC_CORE_IC                 IfxAdc_TmadcSarCore_0   /* sequential after Ia */

#define ADC_RESREG_IA               IfxAdc_TmadcResultReg_0
#define ADC_RESREG_IB               IfxAdc_TmadcResultReg_1
#define ADC_RESREG_IC               IfxAdc_TmadcResultReg_2

/*─────────────────────────────────────────────────────────────────────────────
 * VOLTAGE CHANNELS
 * Va/Vc share SARcore0, Vb/Vdc share SARcore1.
 *───────────────────────────────────────────────────────────────────────────*/
#define ADC_CH_VA                   IfxAdc_TmadcChannel_3
#define ADC_CH_VB                   IfxAdc_TmadcChannel_4
#define ADC_CH_VC                   IfxAdc_TmadcChannel_5
#define ADC_CH_VDC                  IfxAdc_TmadcChannel_6

#define ADC_CORE_VA                 IfxAdc_TmadcSarCore_0
#define ADC_CORE_VB                 IfxAdc_TmadcSarCore_1
#define ADC_CORE_VC                 IfxAdc_TmadcSarCore_0   /* after Va */
#define ADC_CORE_VDC                IfxAdc_TmadcSarCore_1   /* after Vb, parallel with Vc */

#define ADC_RESREG_VA               IfxAdc_TmadcResultReg_3
#define ADC_RESREG_VB               IfxAdc_TmadcResultReg_4
#define ADC_RESREG_VC               IfxAdc_TmadcResultReg_5
#define ADC_RESREG_VDC              IfxAdc_TmadcResultReg_6

/*─────────────────────────────────────────────────────────────────────────────
 * EGTM TRIGGER ROUTING
 * Both groups use the same EGTM source. GROUP_VOLT adds a hardware delay.
 * Verify ADC_EGTM_TRIG_SIGNAL and ADC_TMADC_TRIG_MUXSEL_CURR in your RM:
 *   EGTM chapter → "ADC trigger output matrix" → find your TOM cluster+channel
 *   TMADC chapter → "Trigger input selection"  → confirm mux index
 *───────────────────────────────────────────────────────────────────────────*/
#define ADC_EGTM_CLUSTER         IfxEgtm_Cluster_0
#define ADC_EGTM_TRIG_SOURCE     IfxEgtm_TrigSource_atom
#define ADC_EGTM_TRIG_CHANNEL    IfxEgtm_TrigChannel_0
#define ADC_EGTM_TRIG_SIGNAL      IfxEgtm_Cfg_AdcTriggerSignal_0   /* ADC_OUT0, SEL0 */
#define ADC_TMADC_TRIG_MUXSEL     IfxAdc_TmadcTriggerMuxSel_36     /* TRIG[47:36] offset: 36+0=36 */

/** Current group: sample exactly at PWM valley, no delay */
#define ADC_CURR_TRIGGER_DELAY_NS   (0.0f)

/**
 * Voltage group: delayed 2000 ns after valley.
 * Ensures currents complete fully before voltages start competing for
 * the trigger/result machinery, and places the voltage sample in the
 * middle of the flattest region of the reconstructed phase voltage.
 */
#define ADC_VOLT_TRIGGER_DELAY_NS   (2000.0f)

/*─────────────────────────────────────────────────────────────────────────────
 * SAMPLING TIMES
 *───────────────────────────────────────────────────────────────────────────*/

/**
 * Current channels: 300 ns.
 * Motor current source impedance (shunt + opamp output) is low, settles fast.
 * 300 ns = ~24 TMADC clocks at 80 MHz, well above the 14-clock minimum.
 */
#define ADC_CURR_SAMPLING_TIME_NS   (300.0f)

/**
 * Voltage channels: 500 ns.
 * Phase voltage divider (resistive, typically 10k-100k) has higher source
 * impedance → needs longer sampling time to charge the S/H capacitor fully.
 * 500 ns = ~40 TMADC clocks. Adjust upward if your divider > 50 kΩ.
 */
#define ADC_VOLT_SAMPLING_TIME_NS   (500.0f)

/*─────────────────────────────────────────────────────────────────────────────
 * DMA CONFIGURATION
 * Use two separate DMA channels so current and voltage transfers never block
 * each other. Current DMA has higher hardware priority (lower channel number).
 *───────────────────────────────────────────────────────────────────────────*/
#define ADC_DMA_CURR_CHANNEL        IfxDma_ChannelId_1   /* higher HW priority */
#define ADC_DMA_VOLT_CHANNEL        IfxDma_ChannelId_2
#define ADC_DMA_MODULE              IfxDma_Index_0

/** Buffer sizes: one word per channel per conversion cycle */
#define ADC_DMA_CURR_BUF_SIZE       (3U)    /* Ia, Ib, Ic */
#define ADC_DMA_VOLT_BUF_SIZE       (4U)    /* Va, Vb, Vc, Vdc */

/*─────────────────────────────────────────────────────────────────────────────
 * ISR PRIORITIES
 * Current ISR must be higher priority than voltage ISR.
 * Both must be higher than the FOC background task.
 *───────────────────────────────────────────────────────────────────────────*/
#define IFX_INTPRIO_ADC_CURR    50U    /* current group ISR — highest of the three */
#define IFX_INTPRIO_ADC_VOLT    48U    /* voltage group ISR */

/*─────────────────────────────────────────────────────────────────────────────
 * SCALING — adapt to your hardware
 *───────────────────────────────────────────────────────────────────────────*/

/** Peak current in Amperes at ADC full scale (0xFFF).
 *  Example: 50A shunt/opamp range, unipolar offset shifted to midpoint. */
#define ADC_CURRENT_FULL_SCALE_A    (50.0f)

/** Raw ADC value corresponding to zero current (bipolar offset = Vref/2) */
#define ADC_ZERO_CURRENT_RAW        (2048U)

/** DC bus full-scale voltage at ADC full scale.
 *  Example: 800V bus, 1/200 resistor divider → Vadc_fs = 4.0 V → Vbus_fs = 800 V */
#define ADC_VDC_FULL_SCALE_V        (800.0f)

/** Phase voltage full-scale (typically same divider ratio as DC bus) */
#define ADC_VPHASE_FULL_SCALE_V     (800.0f)

/** Convert raw 12-bit current sample to float Amperes */
#define ADC_RAW_TO_AMPS(raw) \
    (((float32)((raw) >> 4U) - (float32)ADC_ZERO_CURRENT_RAW) * \
     (ADC_CURRENT_FULL_SCALE_A / (float32)ADC_ZERO_CURRENT_RAW))

/** Convert raw 12-bit voltage sample to float Volts (unipolar, no offset) */
#define ADC_RAW_TO_VOLTS_PHASE(raw) \
    ((float32)((raw) >> 4U) * (ADC_VPHASE_FULL_SCALE_V / 4095.0f))

#define ADC_RAW_TO_VOLTS_DC(raw) \
    ((float32)((raw) >> 4U) * (ADC_VDC_FULL_SCALE_V / 4095.0f))

/*─────────────────────────────────────────────────────────────────────────────
 * IIR LOW-PASS FILTER COEFFICIENTS
 *
 *  y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
 *
 *  Cutoff: fc ≈ fsw * alpha / (2π)
 *
 *  Current (alpha=0.8):  fc ≈ 20000 * 0.8 / 6.28 ≈ 2.5 kHz  — keep PI bandwidth
 *  Voltage (alpha=0.3):  fc ≈ 20000 * 0.3 / 6.28 ≈ 955 Hz   — suppress PWM ripple
 *  DC link  (alpha=0.1): fc ≈ 20000 * 0.1 / 6.28 ≈ 318 Hz   — smooth slow bus
 *───────────────────────────────────────────────────────────────────────────*/
#define ADC_FILTER_ALPHA_CURR       (0.8f)
#define ADC_FILTER_ALPHA_VPHASE     (0.3f)
#define ADC_FILTER_ALPHA_VDC        (0.1f)

/*─────────────────────────────────────────────────────────────────────────────
 * PUBLIC DATA TYPES
 *───────────────────────────────────────────────────────────────────────────*/

/**
 * @brief  Phase currents — updated every PWM cycle by ISR_CURR.
 *         All three phases available; Ic is independently measured (not Kirchhoff).
 *         Use ia+ib+ic ≈ 0 as a live hardware sanity check.
 */
typedef struct
{
    float32 ia; /**< Phase A current [A], IIR filtered */
    float32 ib; /**< Phase B current [A], IIR filtered */
    float32 ic; /**< Phase C current [A], IIR filtered */
    float32 kirchhoff_err; /**< |ia+ib+ic| [A] — should be < 0.5 A; alert if > 2 A */
    uint32 timestamp; /**< TMADC timestamp [TMADC ticks] for jitter monitoring */
} Adc_PhaseCurrents;

/**
 * @brief  Phase and DC voltages — updated every PWM cycle by ISR_VOLT.
 *         Used by BEMF observer, DC bus feed-forward, and fault detection.
 */
typedef struct
{
    float32 va; /**< Phase A voltage [V], IIR filtered */
    float32 vb; /**< Phase B voltage [V], IIR filtered */
    float32 vc; /**< Phase C voltage [V], IIR filtered */
    float32 vdc; /**< DC link voltage [V], heavily filtered */
} Adc_PhaseVoltages;

/*─────────────────────────────────────────────────────────────────────────────
 * PUBLIC API
 *───────────────────────────────────────────────────────────────────────────*/

/** Initialize TMADC module, both groups, all 7 channels, DMA, EGTM routing. */
void Adc_MotorSense_init (void);

/** Start continuous HW-triggered conversion (call after PWM is running). */
void Adc_MotorSense_start (void);

/** Get latest phase currents (updated by ISR_CURR, every PWM cycle). */
const Adc_PhaseCurrents* Adc_MotorSense_getCurrents (void);

/** Get latest phase + DC voltages (updated by ISR_VOLT, every PWM cycle). */
const Adc_PhaseVoltages* Adc_MotorSense_getVoltages (void);

/** ISR body for current group — call from IFX_INTERRUPT(priority=IFX_INTPRIO_ADC_CURR) */
void Adc_MotorSense_currIsr (void);

/** ISR body for voltage group — call from IFX_INTERRUPT(priority=IFX_INTPRIO_ADC_VOLT) */
void Adc_MotorSense_voltIsr (void);

#endif /* ADC_MOTOR_SENSE_H */
