/**
 * \file foc.h
 * \brief Torque-mode FOC for PMSM on AURIX TC4Dx.
 *
 * Layer stack:
 *
 *   ADC ISR (100 kHz, PWM-centre synchronised)
 *      |-- Foc_isrStep()            <-- everything happens here
 *             |-- Clarke / Park     (foc_transforms)
 *             |-- PI d / PI q       (FocPi)
 *             |-- circle limiter
 *             |-- inverse Park (+ delay compensation)
 *             |-- SVM (min-max injection) -> duty a/b/c
 *
 * Angle convention: uint32_t, one ELECTRICAL revolution = 2^32.
 * This matches encoder_calib's fixed-point angle so no conversion is needed.
 *
 * Provides the three hooks encoder_calib/example_main expect:
 *   Foc_setForcedAngle(), Foc_disableForcedAngle(), Foc_setCurrentSetpoints().
 */

#ifndef FOC_H
#define FOC_H

#include <stdint.h>
#include <stdbool.h>
#include "Configurations.h"
#include "dap_tune.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * Compile-time configuration -- adapt to your drive
 *============================================================================*/

/* Motor (measure/confirm these!) */
#define FOC_RS_OHM          (0.25f)              /* phase resistance          */
#define FOC_LS_H            (120e-6f)            /* phase inductance (Ld=Lq)  */

/* Current sensing: TLE4972 S6, differential */
#define FOC_I_MAX_A         (30.0f)              /* sensor range / trip level */

/* Current-loop bandwidth: f_c = Kp/(2*pi*L). With Td ~ 18.5 us total delay,
 * 60 deg phase margin gives ~4.5 kHz. */
#define FOC_FC_HZ           (4500.0f)

#define TWO_PI  6.28318530717959f

/*==============================================================================
 * Basic types
 *============================================================================*/

typedef struct
{
    float a, b, c;
} FocAbc;

typedef struct
{
    float alpha, beta;
} FocAb;

typedef struct
{
    float d, q;
} FocDq;

typedef struct
{
    float s, c;
} FocSinCos; /* sin/cos pair          */

/*==============================================================================
 * PI controller with clamping anti-windup
 *============================================================================*/

typedef struct
{
    float kp; /* proportional gain  [V/A]                            */
    float ki; /* integral gain      [V/(A*s)]                        */
    float integ; /* integrator state   [V]                              */
    float outMin; /* updated every cycle from the voltage limit          */
    float outMax;
} FocPi;

void FocPi_init (FocPi *pi, float kp, float ki);
void FocPi_reset (FocPi *pi);
float FocPi_update (FocPi *pi, float err, float ts);

/*==============================================================================
 * Transforms (amplitude-invariant / peak convention)
 *============================================================================*/

/** ia, ib measured (TLE4972 on phases A and B); ic = -(ia+ib). */
FocAb Foc_clarke (float ia, float ib);
FocDq Foc_park (FocAb ab, FocSinCos sc);
FocAb Foc_invPark (FocDq dq, FocSinCos sc);

/** sin/cos from u32 electrical angle (LUT + linear interpolation, no libm). */
FocSinCos Foc_sinCosU32 (uint32_t angleU32);

/*==============================================================================
 * Space-vector modulation (min-max zero-sequence injection)
 *============================================================================*/

/**
 * \param vAb   requested stator voltage vector [V]
 * \param vdc   measured DC-link voltage [V]
 * \param duty  out: duty cycles 0..1 for phases a,b,c (0.5 = zero vector)
 *
 * Linear range: |v| <= vdc/sqrt(3). Caller must limit before calling
 * (Foc_limitVoltage does this) -- this function only clamps as last resort.
 */
void Foc_svm (FocAb vAb, float vdc, FocAbc *duty);

/** Circle limiter with d-axis priority. Returns true if it had to limit. */
bool Foc_limitVoltage (FocDq *v, float vmax);

/*==============================================================================
 * Controller object
 *============================================================================*/

typedef enum
{
    FOC_OFF = 0, /* PWM off / all duties 0.5, integrators reset         */
    FOC_RUN /* closed-loop current control                         */
} FocMode;

typedef struct
{
    /* setpoints (written by main / calib layer, read by ISR) */
    volatile float idRef;
    volatile float iqRef;

    /* forced-angle mode used by encoder alignment */
    volatile bool forcedAngleEn;
    volatile uint32_t forcedAngleU32;

    /* controllers */
    FocPi piD;
    FocPi piQ;

    /* measurements / debug (last ISR values) */
    FocDq iDq;
    FocDq vDq;
    FocAbc duty;
    float omegaE; /* electrical speed [rad/s], for delay comp        */
    uint32_t thetaE; /* angle actually used this cycle                  */
} Foc;

extern Foc g_foc;

/*==============================================================================
 * API
 *============================================================================*/

void Foc_init (void);

/**
 * \brief One complete current-loop step. Call from the ADC EOC ISR.
 *
 * \param ia, ib    phase currents [A] (already converted from ADC)
 * \param vdc       DC-link voltage [V]
 * \param thetaE    electrical rotor angle (u32) from EncoderCalib
 * \param omegaE    electrical speed [rad/s] from encoder (for delay comp)
 * \param dutyOut   duty cycles to write into the PWM compare shadow regs
 */
void Foc_isrStep (float ia, float ib, float vdc, uint32_t thetaE, float omegaE, FocAbc *dutyOut);

/* --- hooks expected by encoder_calib / example_main --------------------- */
void Foc_setForcedAngle (uint32_t angleU32);
void Foc_disableForcedAngle (void);
void Foc_setCurrentSetpoints (float idRef, float iqRef);

void Foc_enable (void); /* FOC_RUN  */
void Foc_disable (void); /* FOC_OFF  */

#ifdef __cplusplus
}
#endif

#endif /* FOC_H */
