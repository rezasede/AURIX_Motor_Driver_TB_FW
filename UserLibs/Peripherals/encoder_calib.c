/**
 * \file encoder_calib.c
 * \brief Counts -> electrical angle mapping, Z anchoring, DC alignment.
 *
 * Uses only the public API of encoder_egtm.h. All functions must run in the
 * same context as the encoder driver calls (current loop / control task).
 */

#include "encoder_calib.h"
#include "encoder_egtm.h"
#include "Configurations.h"
#include "UserFunctions.h"
#include "EGTM_ATOM_3_Phase_Inverter_PWM.h"

#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 * Calibration data — the NVM image
 *============================================================================*/

const EncoderCalibration default_cal = {.magic = 1162040881,   // 1162040881 (ASCII: "ECV1")
        .zToDAxisCounts = 1113,         // 0x00000F39
        .direction = 1,            // \x01
        .polePairs = 7,            // \x07 (7 pole pairs = 14 poles)
        .reserved = {0, 0},       // (\x00, \x00)
        .crc = 2249190550    // 1424169175
        };

/*==============================================================================
 * Calibration Process
 *============================================================================*/

/*
 *
 *
 *
 *  */
static volatile EncoderAlignState alignState = ENC_ALIGN_IDLE; /* ISR->main */

static void alignApplyVector (float duty)
{
    updateEgtmAtom3phInvDuty((uint32_t) duty, 0, 0);
}

static void alignReleaseVector (void)
{
    updateEgtmAtom3phInvDuty(0, 0, 0);
}

volatile int32_t dZlog[32];
volatile uint32_t dZn = 0;
int32_t last = 0;
bool have = false;

void calibrationProcess (void)
{
    /* No/corrupt calibration: run the one-time alignment. */
    EncoderAlignConfig acfg = {.applyVector = alignApplyVector, .releaseVector = alignReleaseVector, .rampTimeS = 0.2f,
            .settleTimeS = 2.0f, .standstillRadS = 0.05f, /* mech rad/s                 */
            .polePairs = MotorPolePairs};
    EncoderCalib_startAlignment(&acfg);

    while (1)
    {
        alignState = EncoderCalib_alignmentTask(0.001);

        if (alignState == ENC_ALIGN_DONE)
        {
            EncoderCalibration cal;
            EncoderCalib_getData(&cal); /* <- zToDAxisCounts is in here now */
            break; // Breakpoint here and save the cal structure
        }

        Delay(1);
    }

}

/*==============================================================================
 * Internal state
 *============================================================================*/

/* Fixed-point scale: u32 angle per MECHANICAL count, for one ELECTRICAL turn
 * per CPR/p counts:  Ke = p * 2^32 / CPR.
 * Computed once in 64-bit; for CPR = 4096, p = 4 -> Ke = 2^22 exactly.
 * For non-power-of-two CPR the per-turn rounding error is < 1 count and does
 * NOT accumulate, because the mechanical count is wrapped to [0, CPR) first. */

static struct
{
    EncoderCalibration cal;
    bool calValid;

    bool refValid; /* an electrical-zero reference exists         */
    int32_t refCount; /* encoder count where theta_e == 0            */

    uint32_t kElec; /* p * 2^32 / CPR (rounded)                    */
    uint32_t lastElecAngle;
    bool zAnchored; /* reference comes from Z (vs. alignment)      */

    /* alignment */
    EncoderAlignConfig acfg;
    EncoderAlignState astate;
    float at; /* state timer                                 */
    int32_t alignCount; /* count captured at the aligned position      */
} s;

/*==============================================================================
 * CRC-32 (IEEE 802.3, reflected, poly 0xEDB88320), bytewise — small & fine
 * for a 16-byte struct. Replace with the FCE if you prefer hardware CRC.
 *============================================================================*/
static uint32_t crc32 (const uint8_t *d, uint32_t n)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < n; i++)
    {
        crc ^= d[i];
        for (int b = 0; b < 8; b++)
        {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

static uint32_t calCrc (const EncoderCalibration *c)
{
    return crc32((const uint8_t*) c, (uint32_t) ((const uint8_t*) &c->crc - (const uint8_t*) c));
}

/*==============================================================================
 * Helpers
 *============================================================================*/

static int32_t wrapToCpr (int32_t v)
{
    int32_t m = v % (int32_t) ENC_COUNTS_PER_REV;
    if (m < 0)
    {
        m += (int32_t) ENC_COUNTS_PER_REV;
    }
    return m;
}

static void computeK (uint8_t polePairs)
{
    /* Ke = round(p * 2^32 / CPR) */
    uint64_t k = ((uint64_t) polePairs << 32) + (ENC_COUNTS_PER_REV / 2u);
    s.kElec = (uint32_t) (k / ENC_COUNTS_PER_REV);
}

/*==============================================================================
 * Public API — data handling
 *============================================================================*/

bool EncoderCalib_setData (const EncoderCalibration *cal)
{
    s.calValid = false;
    if ((cal == (void*) 0) || (cal->magic != ENCODER_CALIB_MAGIC) || (cal->crc != calCrc(cal)) || (cal->polePairs == 0u)
            || ((cal->direction != 1) && (cal->direction != -1)) || (cal->zToDAxisCounts < 0)
            || (cal->zToDAxisCounts >= (int32_t) ENC_COUNTS_PER_REV))
    {
        return false;
    }
    s.cal = *cal;
    s.calValid = true;
    computeK(cal->polePairs);
    /* Reference becomes valid at the first Z pulse (see service()).         */
    return true;
}

void EncoderCalib_getData (EncoderCalibration *cal)
{
    s.cal.magic = ENCODER_CALIB_MAGIC;
    s.cal.crc = calCrc(&s.cal);
    *cal = s.cal;
}

/*==============================================================================
 * Public API — runtime angle
 *============================================================================*/

bool EncoderCalib_service (void)
{
    bool newZ = Encoder_Egtm_service();

    if (newZ && s.calValid)
    {
        /* Re-anchor (or first-anchor) the electrical zero to the Z pulse:
         * theta_e == 0 at count == count_at_Z + zToDAxisCounts (mod CPR).   */
        bool zv;
        int32_t pz = Encoder_Egtm_getPositionAtZ(&zv);
        if (zv)
        {
            s.refCount = pz + s.cal.zToDAxisCounts;
            s.refValid = true;
            s.zAnchored = true;
            return true;
        }
    }
    return false;
}

uint32_t EncoderCalib_getElecAngle (bool *valid)
{
    if (!s.refValid || !s.calValid)
    {
        if (valid != (void*) 0)
        {
            *valid = false;
        }
        return 0u;
    }

    int32_t m = wrapToCpr(Encoder_Egtm_getPosition() - s.refCount);
    uint32_t th = (uint32_t) m * s.kElec; /* wraps naturally in uint32    */

    if (s.cal.direction < 0)
    {
        th = 0u - th;
    }

    s.lastElecAngle = th;
    if (valid != (void*) 0)
    {
        *valid = true;
    }
    return th;
}

float EncoderCalib_getElecSpeed (float periodSeconds)
{
    static uint32_t last = 0u;
    uint32_t now = s.lastElecAngle;
    int32_t delta = (int32_t) (now - last); /* wrap-safe                   */
    last = now;
    return ((float) delta * 1.46291808e-9f) /* 2*pi / 2^32                 */
    / periodSeconds;
}

/*==============================================================================
 * Alignment state machine
 *
 * Sequence:  RAMP    — vector at theta_e = 0, magnitude 0 -> 1 over rampTimeS
 *            SETTLE  — full current, wait standstill for settleTimeS
 *            CAPTURE — encoder reference := here (theta_e = 0), vector off;
 *                      closed-loop FOC may start now
 *            DONE    — a Z pulse was observed afterwards ->
 *                      zToDAxisCounts measured and stored in s.cal
 *============================================================================*/

void EncoderCalib_startAlignment (const EncoderAlignConfig *cfg)
{
    s.acfg = *cfg;
    s.astate = ENC_ALIGN_RAMP;
    s.at = 0.0f;

    s.cal.polePairs = cfg->polePairs;
    s.cal.direction = 1; /* see header: keep sign in one place */
    computeK(cfg->polePairs);
}

EncoderAlignState EncoderCalib_alignmentTask (float periodSeconds)
{
    switch (s.astate)
    {
        case ENC_ALIGN_RAMP :
        {
            s.at += periodSeconds;
            float r = s.at / s.acfg.rampTimeS;
            if (r >= 1.0f)
            {
                r = 1.0f;
                s.astate = ENC_ALIGN_SETTLE;
                s.at = 0.0f;
            }
            s.acfg.applyVector(r * 100.0); /* forced theta_e = 0          */
            break;
        }

        case ENC_ALIGN_SETTLE :
        {
            s.acfg.applyVector(100.0);
            float w = Encoder_Egtm_updateSpeed(periodSeconds); /* mech rad/s    */
            if ((w > -s.acfg.standstillRadS) && (w < s.acfg.standstillRadS))
            {
                s.at += periodSeconds;
                if (s.at >= s.acfg.settleTimeS)
                {
                    /* Rotor d-axis is now on the phase-U axis: theta_e = 0 here */
                    s.alignCount = Encoder_Egtm_getPosition();
                    s.refCount = s.alignCount;
                    s.refValid = true;
                    s.zAnchored = false;
                    s.calValid = true; /* polePairs/direction set at start   */

                    s.acfg.releaseVector();
                    s.astate = ENC_ALIGN_CAPTURE;
                    s.at = 0.0f;
                }
            }
            else
            {
                s.at = 0.0f; /* still moving: restart settle timer  */
            }
            break;
        }

        case ENC_ALIGN_CAPTURE :
        {
            /* Drive may run closed loop already. We wait for a Z pulse to
             * measure zToDAxisCounts (rotate slowly by hand or with small Iq).  */
            if (Encoder_Egtm_service())
            {
                bool zv;
                int32_t pz = Encoder_Egtm_getPositionAtZ(&zv);
                if (zv)
                {
                    /* theta_e = 0 at alignCount  =>
                     * zToDAxisCounts = (alignCount - count_at_Z) mod CPR        */
                    s.cal.zToDAxisCounts = wrapToCpr(s.alignCount - pz);
                    s.astate = ENC_ALIGN_DONE;
                }
            }
            break;
        }

        case ENC_ALIGN_DONE :
        case ENC_ALIGN_FAILED :
        case ENC_ALIGN_IDLE :
        default :
            break;
    }

    return s.astate;
}
