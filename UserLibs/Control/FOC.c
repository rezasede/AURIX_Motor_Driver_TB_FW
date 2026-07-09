/**
 * \file foc.c
 * \brief Torque-mode FOC implementation. No libm calls in the ISR path.
 *
 * All ISR-path math is single-precision float (TC4Dx FPU) plus one LUT.
 */

#include "foc.h"
#include "Configurations.h"
#include "Control_Loops.h"

/*==============================================================================
 * sin/cos lookup table: 256 entries + linear interpolation
 *
 * u32 angle -> index: top 8 bits select the entry, next 8 bits interpolate.
 * Max interpolation error ~ (2*pi/256)^2 / 8 = 7.5e-5 -> plenty for FOC.
 * cos(x) = sin(x + 2^30) (quarter turn in u32 space) -> one table serves both.
 *============================================================================*/

#define SINLUT_BITS   (8u)
#define SINLUT_SIZE   (1u << SINLUT_BITS)              /* 256                 */

static float sinLut[SINLUT_SIZE + 1]; /* +1: wrap-free interp*/

static float sinU32 (uint32_t a)
{
    uint32_t idx = a >> (32u - SINLUT_BITS); /* 0..255    */
    uint32_t frac = (a >> (32u - 2u * SINLUT_BITS)) & 0xFFu; /* 0..255    */
    float f = (float) frac * (1.0f / 256.0f);
    return sinLut[idx] + f * (sinLut[idx + 1u] - sinLut[idx]);
}

FocSinCos Foc_sinCosU32 (uint32_t angleU32)
{
    FocSinCos sc;
    sc.s = sinU32(angleU32);
    sc.c = sinU32(angleU32 + 0x40000000u); /* +90 deg electrical            */
    return sc;
}

/*==============================================================================
 * Transforms -- amplitude-invariant (peak) convention:
 *   id/iq have the same amplitude as the phase current peak,
 *   torque T = 1.5 * p * lambda_m * iq.
 *============================================================================*/

#define ONE_OVER_SQRT3   (0.57735026919f)

FocAb Foc_clarke (float ia, float ib)
{
    /* ic is not measured: ic = -(ia+ib), which folds into:
     *   i_alpha = ia
     *   i_beta  = (ia + 2*ib) / sqrt(3)                                      */
    FocAb ab;
    ab.alpha = ia;
    ab.beta = (ia + 2.0f * ib) * ONE_OVER_SQRT3;
    return ab;
}

FocDq Foc_park (FocAb ab, FocSinCos sc)
{
    FocDq dq;
    dq.d = ab.alpha * sc.c + ab.beta * sc.s;
    dq.q = -ab.alpha * sc.s + ab.beta * sc.c;
    return dq;
}

FocAb Foc_invPark (FocDq dq, FocSinCos sc)
{
    FocAb ab;
    ab.alpha = dq.d * sc.c - dq.q * sc.s;
    ab.beta = dq.d * sc.s + dq.q * sc.c;
    return ab;
}

/*==============================================================================
 * PI with clamping anti-windup
 *
 * Discrete form:  u = kp*e + integ;  integ += ki*Ts*e  (forward Euler)
 * Anti-windup:    integrator only accumulates while the un-clamped output is
 *                 inside [outMin, outMax] OR the error drives it back inside.
 *============================================================================*/

void FocPi_init (FocPi *pi, float kp, float ki)
{
    pi->kp = kp;
    pi->ki = ki;
    pi->integ = 0.0f;
    pi->outMin = 0.0f;
    pi->outMax = 0.0f;
}

void FocPi_reset (FocPi *pi)
{
    pi->integ = 0.0f;
}

float FocPi_update (FocPi *pi, float err, float ts)
{
    float u = pi->kp * err + pi->integ;

    if (u > pi->outMax)
    {
        u = pi->outMax;
        if (err < 0.0f)
        {
            pi->integ += pi->ki * ts * err;
        } /* unwinding ok  */
    }
    else if (u < pi->outMin)
    {
        u = pi->outMin;
        if (err > 0.0f)
        {
            pi->integ += pi->ki * ts * err;
        }
    }
    else
    {
        pi->integ += pi->ki * ts * err;
    }
    return u;
}

/*==============================================================================
 * Voltage circle limiter, d priority (d keeps flux/field where commanded,
 * q gives up first). vmax = vdc / sqrt(3) for linear SVM.
 *============================================================================*/

bool Foc_limitVoltage (FocDq *v, float vmax)
{
    float magSq = v->d * v->d + v->q * v->q;
    if (magSq <= vmax * vmax)
    {
        return false;
    }

    /* clamp d first, then give q the remaining headroom */
    if (v->d > vmax)
    {
        v->d = vmax;
    }
    if (v->d < -vmax)
    {
        v->d = -vmax;
    }

    float qHeadSq = vmax * vmax - v->d * v->d;
    float qHead = 0.0f;
    if (qHeadSq > 0.0f)
    {
        /* fast inverse-free sqrt via Newton (2 iterations, seed vmax/2) */
        float x = 0.5f * vmax;
        x = 0.5f * (x + qHeadSq / x);
        x = 0.5f * (x + qHeadSq / x);
        qHead = x;
    }
    if (v->q > qHead)
    {
        v->q = qHead;
    }
    if (v->q < -qHead)
    {
        v->q = -qHead;
    }
    return true;
}

/*==============================================================================
 * SVM via min-max (symmetric zero-sequence) injection.
 *
 * Identical output to classical sector-based SVPWM, no sector logic:
 *   1. inverse Clarke -> va, vb, vc          (phase voltages, peak conv.)
 *   2. vzs = -(max+min)/2                    (centres the envelope)
 *   3. duty_x = 0.5 + (vx + vzs)/vdc
 *
 * Linear until |v| = vdc/sqrt(3)  (15.5% more than sine PWM's vdc/2).
 *============================================================================*/

void Foc_svm (FocAb vAb, float vdc, FocAbc *duty)
{
    /* inverse Clarke (amplitude-invariant) */
    float va = vAb.alpha;
    float vb = -0.5f * vAb.alpha + (0.86602540378f) * vAb.beta; /* sqrt3/2 */
    float vc = -0.5f * vAb.alpha - (0.86602540378f) * vAb.beta;

    /* min-max zero-sequence */
    float vMax = va;
    float vMin = va;
    if (vb > vMax)
    {
        vMax = vb;
    }
    if (vb < vMin)
    {
        vMin = vb;
    }
    if (vc > vMax)
    {
        vMax = vc;
    }
    if (vc < vMin)
    {
        vMin = vc;
    }
    float vzs = -0.5f * (vMax + vMin);

    float invVdc = (vdc > 1.0f) ? (1.0f / vdc) : 1.0f; /* guard vdc ~ 0     */

    duty->a = 0.5f + (va + vzs) * invVdc;
    duty->b = 0.5f + (vb + vzs) * invVdc;
    duty->c = 0.5f + (vc + vzs) * invVdc;

    /* last-resort clamp (over-modulation) */
    if (duty->a < 0.0f)
    {
        duty->a = 0.0f;
    }
    if (duty->a > 1.0f)
    {
        duty->a = 1.0f;
    }
    if (duty->b < 0.0f)
    {
        duty->b = 0.0f;
    }
    if (duty->b > 1.0f)
    {
        duty->b = 1.0f;
    }
    if (duty->c < 0.0f)
    {
        duty->c = 0.0f;
    }
    if (duty->c > 1.0f)
    {
        duty->c = 1.0f;
    }
}

/*==============================================================================
 * Controller
 *============================================================================*/

Foc g_foc;

/* minimal sin() for LUT generation at init (Taylor to x^7, folded to [0,pi/2])
 * -- runs once at boot, accuracy 3e-7, avoids pulling in libm.               */
static float initSin (float x)
{
    const float PI = 3.14159265358979f;
    float s = 1.0f;
    while (x >= 2.0f * PI)
    {
        x -= 2.0f * PI;
    }
    if (x >= PI)
    {
        x -= PI;
        s = -1.0f;
    }
    if (x > 0.5f * PI)
    {
        x = PI - x;
    }
    float x2 = x * x;
    return s * x * (1.0f + x2 * (-1.0f / 6.0f + x2 * (1.0f / 120.0f - x2 / 5040.0f)));
}

void Foc_init (void)
{
    for (uint32_t i = 0u; i <= SINLUT_SIZE; i++)
    {
        sinLut[i] = initSin(TWO_PI * (float) i / (float) SINLUT_SIZE);
    }

    /* Gains from plant inversion:
     *   Kp = 2*pi*f_c*L          -> f_c = Kp/(2*pi*L)
     *   Ki = Kp * R/L            -> controller zero cancels plant pole R/L
     * Numbers: L=120uH, R=0.25R, f_c=4.5kHz
     *   Kp = 2*pi*4500*120e-6 = 3.39 V/A
     *   Ki = 3.39 * 0.25/120e-6 = 7069 V/(A*s)                               */
    float kp = TWO_PI * FOC_FC_HZ * FOC_LS_H;
    float ki = kp * (FOC_RS_OHM / FOC_LS_H);

    FocPi_init(&g_foc.piD, kp, ki);
    FocPi_init(&g_foc.piQ, kp, ki);

    g_foc.idRef = 0.0f;
    g_foc.iqRef = 0.0f;
    g_foc.forcedAngleEn = false;
    g_foc.forcedAngleU32 = 0u;

    Foc_disable();
    CTRL_S = initial;
}

/*------------------------------------------------------------------------*/
/* Hooks used by encoder_calib alignment (see example_main.c)             */
/*------------------------------------------------------------------------*/

void Foc_setForcedAngle (uint32_t angleU32)
{
    g_foc.forcedAngleU32 = angleU32;
    g_foc.forcedAngleEn = true;
}

void Foc_disableForcedAngle (void)
{
    g_foc.forcedAngleEn = false;
}

void Foc_setCurrentSetpoints (float idRef, float iqRef)
{
    g_foc.idRef = idRef;
    g_foc.iqRef = iqRef;
}

void Foc_disable (void)
{
    CTRL_S = initial;
    FocPi_reset(&g_foc.piD);
    FocPi_reset(&g_foc.piQ);
}

/*------------------------------------------------------------------------*/
/* The ISR step                                                           */
/*------------------------------------------------------------------------*/

void Foc_isrStep (float ia, float ib, float vdc, uint32_t thetaE, float omegaE, FocAbc *dutyOut)
{
    if (CTRL_S != running)
    {
        dutyOut->a = 0.5f;
        dutyOut->b = 0.5f;
        dutyOut->c = 0.5f;
        g_foc.duty = *dutyOut;
        return;
    }

    /* 1. angle selection: forced during alignment, encoder otherwise ------*/
    uint32_t theta = g_foc.forcedAngleEn ? g_foc.forcedAngleU32 : thetaE;
    g_foc.thetaE = theta;
    g_foc.omegaE = omegaE;

    /* 2. Clarke + Park on the MEASUREMENT angle ---------------------------*/
    FocSinCos scMeas = Foc_sinCosU32(theta);
    FocAb iAb = Foc_clarke(ia, ib);
    FocDq iDq = Foc_park(iAb, scMeas);
    g_foc.iDq = iDq;

    /* 3. voltage limit for this cycle: linear SVM hexagon inscribed circle */
    float vmax = vdc * ONE_OVER_SQRT3;
    g_foc.piD.outMin = -vmax;
    g_foc.piD.outMax = vmax;
    g_foc.piQ.outMin = -vmax;
    g_foc.piQ.outMax = vmax;

    /* 4. PI current controllers -------------------------------------------*/
    FocDq vDq;
    vDq.d = FocPi_update(&g_foc.piD, g_foc.idRef - iDq.d, PWM_Period);
    vDq.q = FocPi_update(&g_foc.piQ, g_foc.iqRef - iDq.q, PWM_Period);

    /* 5. cross-coupling decoupling feedforward ----------------------------
     *   vd += -we*Lq*iq ; vq += we*(Ld*id + lambda_m)
     * lambda_m (back-EMF) term left out until measured -- at low speed the
     * PI absorbs it; add it before spinning fast.                           */
    vDq.d += -omegaE * FOC_LS_H * iDq.q;
    vDq.q += omegaE * FOC_LS_H * iDq.d;

    /* 6. keep the vector inside the linear region -------------------------*/
    (void) Foc_limitVoltage(&vDq, vmax);
    g_foc.vDq = vDq;

    /* 7. inverse Park on the ADVANCED angle --------------------------------
     * The duty computed now becomes voltage centred 1.5 periods after the
     * sample (0.5 Ts to shadow-load + 1.0 Ts averaging). Rotate the output
     * vector forward by we * 1.5 * Ts so it lands where the rotor WILL be.
     * u32 conversion: dTheta[u32] = we * 1.5Ts / (2*pi) * 2^32.             */
    float dThetaTurns = omegaE * (1.5f * PWM_Period) * (1.0f / 6.28318530718f);
    uint32_t thetaAdv = theta + (uint32_t) (int32_t) (dThetaTurns * 4294967296.0f);

    FocSinCos scOut = Foc_sinCosU32(thetaAdv);
    FocAb vAb = Foc_invPark(vDq, scOut);

    /* 8. SVM -> duties ------------------------------------------------------*/
    Foc_svm(vAb, vdc, dutyOut);
    g_foc.duty = *dutyOut;
}
