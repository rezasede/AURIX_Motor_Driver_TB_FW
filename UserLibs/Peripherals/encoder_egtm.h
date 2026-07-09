/**
 * \file encoder_egtm.h
 * \brief Incremental encoder (A/B/Z) driver for AURIX TC4Dx.
 *        Hardware x4 quadrature decode using eGTM TIM0 + SPE0 (zero CPU load).
 *
 * ============================================================================
 *  VERIFIED against official documents (no guessed registers):
 *    - AURIX TC4Dx User Manual v1.1, 2025-06-26, chapter 44 (eGTM),
 *      incl. section 44.9 "TC4Dx eGTM information"
 *    - IfxEgtm_PinMap_TC4Dx_BGA292_COM.c (iLLD-TC4 v2.4.0)
 *
 *  Device facts that define this design (UM 44.9.1, Table 2363):
 *    - TC4Dx eGTM has NO MCS, NO ARU, NO DPLL, NO GPT12.
 *      (A GTM-classic style MCS/ARU decoder is impossible on this device.)
 *    - TC4Dx eGTM HAS 3x SPE (Sensor Pattern Evaluation) with a 24-bit
 *      hardware up/down counter CLS0_SPE_REV_CNT: +1 per valid pattern in
 *      forward direction, -1 in backward direction  ->  hardware quadrature.
 * ============================================================================
 *
 * Signal path:
 *
 *   ENC_B  P00.10 --TIM0INSEL.CH1SEL=1--> TIM0 CH1 --filter--> SPE0 input y
 *   ENC_A  P00.11 --TIM0INSEL.CH2SEL=1--> TIM0 CH2 --filter--> SPE0 input z
 *   (SPE0 input x = TIM0 CH0: unused, masked via SIE0=0, sampled as 0)
 *   ENC_Z  P00.12 --TIM0INSEL.CH3SEL=1--> TIM0 CH3 (TIEM, rising edge)
 *                                          -> CNT = hardware rev counter
 *                                          -> GPR0 = TBU_TS0 timestamp of Z
 *
 *   SPE0 pattern table = the 4 quadrature states programmed twice
 *   (8 entries -> the 3-bit pattern pointer wraps consistently mod 8).
 *   SPE validates every transition (illegal jump -> PERR interrupt flag),
 *   detects direction in hardware and counts REV_CNT up/down.
 *
 *   Position (x4 counts) = CLS0_SPE_REV_CNT, 24-bit, extended to 32-bit
 *   in software by this driver.
 *
 * No interrupts are required for position/speed. Optionally the Z channel
 * NEWVAL interrupt can be enabled for index-synchronised actions.
 */

#ifndef ENCODER_EGTM_H
#define ENCODER_EGTM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * User configuration
 *============================================================================*/

/** Encoder line count (A/B pulses per mechanical revolution). */
#ifndef ENC_PULSES_PER_REV
#define ENC_PULSES_PER_REV      (1200u)
#endif

/** Counts per mechanical revolution with x4 decoding. */
#define ENC_COUNTS_PER_REV      (4u * ENC_PULSES_PER_REV)

/**
 * TIM input glitch filter threshold for A/B/Z in CMU_CLK_RES0 ticks.
 * With the default clock setup of this driver, one tick = one eGTM cluster-0
 * clock period (fEGTM/2 with the reset-default cluster divider /2 kept).
 * Rule of thumb: filter time < 1/4 of the minimum quadrature edge spacing.
 * Edge rate = 4 * PPR * RPM / 60.
 * Example: fEGTM=200MHz -> tick=10ns; 1024 PPR @ 6000 RPM -> 409.6 kHz edges
 * -> 2.44 us spacing -> filter <= 0.6 us -> <= 60 ticks.
 */
#ifndef ENC_TIM_FILTER_TICKS
#define ENC_TIM_FILTER_TICKS    (50u)
#endif

/** Set to 1 to invert the positive counting direction (equivalent to
 *  swapping A and B). Implemented by reversing the SPE pattern sequence. */
#ifndef ENC_INVERT_DIRECTION
#define ENC_INVERT_DIRECTION    (0)
#endif

/*==============================================================================
 * API
 *============================================================================*/

typedef struct
{
    int32_t  position;      /**< x4 counts, 32-bit software-extended          */
    int32_t  revolutions;   /**< Z (index) pulses since init/reset (24-bit HW
                                 counter, unsigned count of index pulses)     */
    uint32_t zTimestamp;    /**< TBU_TS0 timestamp (24-bit) of the last Z     */
    bool     direction;     /**< last hardware-detected direction (SPE ADIR)  */
    uint32_t errorFlags;    /**< bit0 PERR: invalid/illegal pattern transition
                                        (missed edge, noise, overspeed)
                                 bit1 BIS : bouncing input detected
                                 (both are sticky until Encoder_Egtm_clearErrors) */
} Encoder_Egtm_Status;

/**
 * \brief Full hardware init: eGTM module enable (CLC), CMU clock 0,
 *        TIM0INSEL pin muxing, TBU CH0 timebase, TIM0 CH1/CH2/CH3, SPE0
 *        pattern table + enable.
 * \note  Call once after the pads P00.10/11/12 are in input mode (reset
 *        default on AURIX is input/tri-state; add pull-ups/downs to match
 *        your encoder's output stage if needed, see README).
 * \return true on success (module enable + readback checks passed).
 */
bool Encoder_Egtm_init(void);

/** \brief Current x4 position in counts (reads SPE0_REV_CNT and extends the
 *         24-bit hardware value to 32-bit).
 *  \note  Must be called from a single context (not reentrant), and at least
 *         once per 2^23 counts of movement (any control loop satisfies this
 *         by orders of magnitude). */
int32_t Encoder_Egtm_getPosition(void);

/** \brief Mechanical shaft angle in [0, 2*pi), relative to the position at
 *         init/reset (align to Z using getStatus().revolutions if needed). */
float Encoder_Egtm_getAngleRad(void);

/** \brief Snapshot of position, revolution counter, Z timestamp, direction
 *         and sticky error flags. */
void Encoder_Egtm_getStatus(Encoder_Egtm_Status *status);

/** \brief Zero the position and revolution reference (pure software rebase,
 *         no hardware registers are written -> race-free at any speed). */
void Encoder_Egtm_reset(void);

/** \brief Clear the sticky PERR/BIS error flags. */
void Encoder_Egtm_clearErrors(void);

/**
 * \brief M-method speed estimate. Call at a fixed period from your control
 *        loop; returns rad/s from the position delta since the last call.
 */
float Encoder_Egtm_updateSpeed(float periodSeconds);

/**
 * \brief Optionally enable the TIM0 CH3 NEWVAL interrupt (one IRQ per index
 *        pulse). Routing the eGTM service request to a CPU via the Interrupt
 *        Router and providing the ISR remains application responsibility.
 */
void Encoder_Egtm_enableZInterrupt(void);

/*------------------------------------------------------------------------------
 * Z-latch service — required by the calibration layer (encoder_calib.c)
 *----------------------------------------------------------------------------*/

/**
 * \brief Poll for a new index pulse and latch the position at which it
 *        occurred. Call this once per current-loop period (same context as
 *        all other position reads), BEFORE reading the electrical angle.
 *
 *        Latency note: the latched value lags the true Z position by at most
 *        one call period times the speed. At a 10 kHz loop and 6000 RPM with
 *        1024 PPR that is <= 41 counts; the calibration layer removes this
 *        systematic part statistically (see encoder_calib.c). If you enable
 *        the Z interrupt instead, call Encoder_Egtm_notifyZ() from the ISR
 *        and skip the polling for a jitter of only a few counts.
 *
 * \return true if a new Z pulse was detected in this call.
 */
bool Encoder_Egtm_service(void);

/** \brief Z-latch from ISR context (alternative to polling via _service). */
void Encoder_Egtm_notifyZ(void);

/** \brief Last latched position at Z. \param valid false until the first Z. */
int32_t Encoder_Egtm_getPositionAtZ(bool *valid);

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_EGTM_H */
