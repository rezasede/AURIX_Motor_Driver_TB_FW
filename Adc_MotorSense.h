/**
 * @file    Adc_MotorSense.h
 * @brief   10-channel unified TMADC motor sensing — 3×differential current + 3×phase voltage + Vdc
 *
 * Architecture overview:
 * ─────────────────────────────────────────────────────────────────────────
 *  Each TLE4972 provides two analog outputs (VOUTP / VOUTN), differential.
 *  Both outputs of every sensor feed dedicated TMADC channels.
 *  ALL 10 channels belong to ONE group sharing ONE trigger source.
 *
 *  The EGTM ATOM Ch3 rising edge fires at the PWM valley (center-aligned
 *  triangle peak), which ENDS sampling on all 10 channels simultaneously.
 *  Each channel has its own S&H capacitor (per RM section 45.4.2), so the
 *  freeze is a true hardware snapshot — zero skew between any two channels.
 *
 *  After the freeze:
 *    Core-0 (5 ch): Ia_P, Ib_P, Ic_P, Va, Vc   — converts in parallel with Core-1
 *    Core-1 (5 ch): Ia_N, Ib_N, Ic_N, Vb, Vdc
 *
 *  One DMA channel transfers all 10 result words (one per SR fire).
 *  One CPU ISR fires after the 10th DMA transfer, computes differentials
 *  and scales results.
 *
 * Channel / result register / DMA buffer layout:
 * ─────────────────────────────────────────────────────────────────────────
 *  Idx  Register  Channel  Core  Content
 *   0   RES[0]    CH0      0     Ia_P  (TLE4972 Phase-A VOUTP)
 *   1   RES[1]    CH1      1     Ia_N  (TLE4972 Phase-A VOUTN)
 *   2   RES[2]    CH2      0     Ib_P  (TLE4972 Phase-B VOUTP)
 *   3   RES[3]    CH3      1     Ib_N  (TLE4972 Phase-B VOUTN)
 *   4   RES[4]    CH4      0     Ic_P  (TLE4972 Phase-C VOUTP)
 *   5   RES[5]    CH5      1     Ic_N  (TLE4972 Phase-C VOUTN)
 *   6   RES[6]    CH6      0     Va    (Phase-A voltage divider)
 *   7   RES[7]    CH7      1     Vb    (Phase-B voltage divider)
 *   8   RES[8]    CH8      0     Vc    (Phase-C voltage divider)
 *   9   RES[9]    CH9      1     Vdc   (DC link voltage divider)
 *
 * tHOLD check (datasheet max = 3 µs at 5 V):
 *   Each core has 5 channels. Worst-case last channel hold time:
 *   4 × tconv = 4 × 200 ns = 800 ns  <<  3 µs  ✓
 *
 * Timing budget at 200 kHz PWM (T = 5 µs, valid window ≈ 2.5 µs):
 *   tmstc (sampling)          100 ns
 *   Hold + convert (5 ch/core) 5 × 200 ns = 1000 ns  (both cores parallel)
 *   DMA transfers              10 × ~20 ns ≈  200 ns
 *   ISR math                               ≈  100 ns
 *   Total                                  ≈ 1400 ns  << 2500 ns  ✓
 */

#ifndef ADC_MOTOR_SENSE_H
#define ADC_MOTOR_SENSE_H

#include "Ifx_Types.h"
#include "Adc/Tmadc/IfxAdc_Tmadc.h"
#include "Egtm/Trigger/IfxEgtm_Trigger.h"
#include "Dma/Dma/IfxDma_Dma.h"
#include "EGTM_ATOM_3_Phase_Inverter_PWM.h"

/*══════════════════════════════════════════════════════════════════════════
 * TMADC MODULE
 *══════════════════════════════════════════════════════════════════════════*/
#define ADC_TMADC_MODULE            IfxAdc_TmadcModule_0

/*══════════════════════════════════════════════════════════════════════════
 * CURRENT CHANNELS — TLE4972 differential pairs
 *   P-side (VOUTP) → Core-0 → channels 0, 2, 4  (even)
 *   N-side (VOUTN) → Core-1 → channels 1, 3, 5  (odd)
 *══════════════════════════════════════════════════════════════════════════*/
#define ADC_CH_IA_P                 IfxAdc_TmadcChannel_0
#define ADC_CH_IA_N                 IfxAdc_TmadcChannel_1
#define ADC_CH_IB_P                 IfxAdc_TmadcChannel_2
#define ADC_CH_IB_N                 IfxAdc_TmadcChannel_3
#define ADC_CH_IC_P                 IfxAdc_TmadcChannel_4
#define ADC_CH_IC_N                 IfxAdc_TmadcChannel_5

#define ADC_CORE_IA_P               IfxAdc_TmadcSarCore_0
#define ADC_CORE_IA_N               IfxAdc_TmadcSarCore_1
#define ADC_CORE_IB_P               IfxAdc_TmadcSarCore_0
#define ADC_CORE_IB_N               IfxAdc_TmadcSarCore_1
#define ADC_CORE_IC_P               IfxAdc_TmadcSarCore_0
#define ADC_CORE_IC_N               IfxAdc_TmadcSarCore_1

/*══════════════════════════════════════════════════════════════════════════
 * VOLTAGE CHANNELS — single-ended, alternating cores for load balance
 *   Core-0: Va(6), Vc(8)
 *   Core-1: Vb(7), Vdc(9)
 *══════════════════════════════════════════════════════════════════════════*/
#define ADC_CH_VA                   IfxAdc_TmadcChannel_6
#define ADC_CH_VB                   IfxAdc_TmadcChannel_7
#define ADC_CH_VC                   IfxAdc_TmadcChannel_8
#define ADC_CH_VDC                  IfxAdc_TmadcChannel_9

#define ADC_CORE_VA                 IfxAdc_TmadcSarCore_0
#define ADC_CORE_VB                 IfxAdc_TmadcSarCore_1
#define ADC_CORE_VC                 IfxAdc_TmadcSarCore_0
#define ADC_CORE_VDC                IfxAdc_TmadcSarCore_1

/*══════════════════════════════════════════════════════════════════════════
 * RESULT REGISTERS — consecutive from RES[0], interleaved P/N
 *══════════════════════════════════════════════════════════════════════════*/
#define ADC_RESREG_IA_P             IfxAdc_TmadcResultReg_0
#define ADC_RESREG_IA_N             IfxAdc_TmadcResultReg_1
#define ADC_RESREG_IB_P             IfxAdc_TmadcResultReg_2
#define ADC_RESREG_IB_N             IfxAdc_TmadcResultReg_3
#define ADC_RESREG_IC_P             IfxAdc_TmadcResultReg_4
#define ADC_RESREG_IC_N             IfxAdc_TmadcResultReg_5
#define ADC_RESREG_VA               IfxAdc_TmadcResultReg_6
#define ADC_RESREG_VB               IfxAdc_TmadcResultReg_7
#define ADC_RESREG_VC               IfxAdc_TmadcResultReg_8
#define ADC_RESREG_VDC              IfxAdc_TmadcResultReg_9

/*══════════════════════════════════════════════════════════════════════════
 * DMA BUFFER INDICES — direct index into g_dmaAll[10]
 *══════════════════════════════════════════════════════════════════════════*/
#define ADC_DMA_IDX_IA_P            (0U)
#define ADC_DMA_IDX_IA_N            (1U)
#define ADC_DMA_IDX_IB_P            (2U)
#define ADC_DMA_IDX_IB_N            (3U)
#define ADC_DMA_IDX_IC_P            (4U)
#define ADC_DMA_IDX_IC_N            (5U)
#define ADC_DMA_IDX_VA              (6U)
#define ADC_DMA_IDX_VB              (7U)
#define ADC_DMA_IDX_VC              (8U)
#define ADC_DMA_IDX_VDC             (9U)

/*══════════════════════════════════════════════════════════════════════════
 * EGTM TRIGGER
 *   ATOM Cluster-0 Ch3 generates a rising edge at the PWM valley.
 *   This single trigger ends sampling on ALL 10 channels simultaneously.
 *   All channels are configured with the same HWTRxMS (via shared grpCfg)
 *   and HWTRDLYEN = 0 — no per-channel delay skew possible.
 *══════════════════════════════════════════════════════════════════════════*/
#define ADC_EGTM_CLUSTER            IfxEgtm_Cluster_0
#define ADC_EGTM_TRIG_SOURCE        IfxEgtm_TrigSource_atom
#define ADC_EGTM_TRIG_CHANNEL       IfxEgtm_TrigChannel_3
#define ADC_EGTM_TRIG_SIGNAL        IfxEgtm_Cfg_AdcTriggerSignal_0
#define ADC_TMADC_TRIG_MUXSEL       IfxAdc_TmadcTriggerMuxSel_36

/*══════════════════════════════════════════════════════════════════════════
 * TRIGGER DELAY — ZERO for every channel
 *   HWTRDLYEN = 0 on all 10 channels enforces that the trigger event
 *   immediately ends sampling with no additional delay on any channel.
 *   Any non-zero delay on even one channel would break the synchronous freeze.
 *══════════════════════════════════════════════════════════════════════════*/
#define ADC_TRIGGER_DELAY_NS        (0.0f)

/*══════════════════════════════════════════════════════════════════════════
 * SAMPLING TIMES
 *   100 ns satisfies tmstc(min) = 50 ns at 5 V (datasheet Table 80)
 *   with margin, and fits the 200 kHz timing budget.
 *══════════════════════════════════════════════════════════════════════════*/
#define ADC_CURR_SAMPLING_TIME_NS   (100.0f)
#define ADC_VOLT_SAMPLING_TIME_NS   (100.0f)

/*══════════════════════════════════════════════════════════════════════════
 * DMA — single channel, 10 transfers per PWM cycle
 *══════════════════════════════════════════════════════════════════════════*/
#define ADC_DMA_CHANNEL             IfxDma_ChannelId_1
#define ADC_DMA_MODULE              IfxDma_Index_0
#define ADC_DMA_BUF_SIZE            (10U)

/*══════════════════════════════════════════════════════════════════════════
 * ISR PRIORITY — single ISR after all 10 DMA transfers complete
 *══════════════════════════════════════════════════════════════════════════*/
#define IFX_INTPRIO_ADC_ALL         (50U)

/*══════════════════════════════════════════════════════════════════════════
 * RESULT REGISTER EXTRACTION
 *   TMADC 32-bit result register layout:
 *     Bit 19:     VF  — Valid Flag (1 = conversion complete, result valid)
 *     Bits[15:4]: 12-bit unsigned conversion result (0..4095)
 *     Other bits: reserved / status
 *══════════════════════════════════════════════════════════════════════════*/
#define ADC_GET_RESULT(word)  ((word) & 0x0FFFU)                 /* bits [11:0] */
#define ADC_GET_CHNR(word)    (((word) >> 16U) & 0x000FU)        /* bits [19:16] */
#define ADC_GET_TRE(word)     (((word) >> 31U) & 0x0001U)        /* bit 31 */

/*══════════════════════════════════════════════════════════════════════════
 * TLE4972 DIFFERENTIAL CURRENT SCALING
 *
 *   The TLE4972 outputs are ratiometric to VDDA:
 *     VOUTP = VDDA/2 + ΔV/2
 *     VOUTN = VDDA/2 - ΔV/2
 *
 *   After subtraction:
 *     DIFF = VOUTP_raw - VOUTN_raw
 *     At  0 A: DIFF =    0  (both outputs at mid-scale 2048)
 *     At +Imax: DIFF = +4095 (VOUTP at full scale, VOUTN at zero)
 *     At -Imax: DIFF = -4095 (VOUTP at zero,       VOUTN at full scale)
 *
 *   Sensitivity (differential):
 *     ADC_CURR_SENS_MV_PER_A = 2 × single-ended sensitivity of the TLE4972.
 *     Example: TLE4972-AE35D5 single-ended ~40 mV/A → differential 80 mV/A.
 *     *** CALIBRATE THIS VALUE against your specific TLE4972 variant ***
 *
 *   Conversion:
 *     counts_per_amp = (sensitivity_mV_per_A / Vref_mV) × 4095
 *     amps = diff_counts / counts_per_amp
 *══════════════════════════════════════════════════════════════════════════*/
#define ADC_CURRENT_FULL_SCALE_A    (50.0f)
#define ADC_VREF_MV                 (5000.0f)
#define ADC_CURR_SENS_MV_PER_A      (80.0f)     /* Calibrate per TLE4972 variant */
#define ADC_CURR_COUNTS_PER_AMP     ((ADC_CURR_SENS_MV_PER_A / ADC_VREF_MV) * 4095.0f)
#define ADC_DIFF_TO_AMPS(diff)      ((float32)(diff) / ADC_CURR_COUNTS_PER_AMP)

/*══════════════════════════════════════════════════════════════════════════
 * VOLTAGE SCALING — single-ended, set by external voltage divider ratios
 *══════════════════════════════════════════════════════════════════════════*/
#define ADC_VPHASE_FULL_SCALE_V     (800.0f)
#define ADC_VDC_FULL_SCALE_V        (800.0f)
#define ADC_RAW_TO_VOLTS_PHASE(raw) ((float32)(raw) * (ADC_VPHASE_FULL_SCALE_V / 4095.0f))
#define ADC_RAW_TO_VOLTS_DC(raw)    ((float32)(raw) * (ADC_VDC_FULL_SCALE_V    / 4095.0f))

/*══════════════════════════════════════════════════════════════════════════
 * DATA TYPES
 *══════════════════════════════════════════════════════════════════════════*/

/**
 * @brief Phase current results — all derived from a single synchronized snapshot.
 */
typedef struct
{
    float32  ia;                    /**< Phase-A current [A]                            */
    float32  ib;                    /**< Phase-B current [A]                            */
    float32  ic;                    /**< Phase-C current [A]                            */
    float32  kirchhoff_err;         /**< ia+ib+ic — should be ~0 A (fault indicator)   */
    sint16    raw_p[3];              /**< Raw P-side counts [0..4095], index=phase       */
    sint16    raw_n[3];              /**< Raw N-side counts [0..4095], common-mode debug */
    sint16    raw_diff[3];           /**< P-N difference  [−4095..+4095]                */
} Adc_PhaseCurrents;

/**
 * @brief Phase and DC-link voltages — sampled at the same instant as currents.
 */
typedef struct
{
    float32  va;                    /**< Phase-A voltage [V] */
    float32  vb;                    /**< Phase-B voltage [V] */
    float32  vc;                    /**< Phase-C voltage [V] */
    float32  vdc;                   /**< DC link voltage [V] */
} Adc_PhaseVoltages;

/*══════════════════════════════════════════════════════════════════════════
 * PUBLIC API
 *══════════════════════════════════════════════════════════════════════════*/
void                      Adc_MotorSense_init(void);
void                      Adc_MotorSense_start(void);
const Adc_PhaseCurrents*  Adc_MotorSense_getCurrents(void);
const Adc_PhaseVoltages*  Adc_MotorSense_getVoltages(void);
void                      Adc_MotorSense_allIsr(void);   /**< Called from Isr.c */

#endif /* ADC_MOTOR_SENSE_H */
