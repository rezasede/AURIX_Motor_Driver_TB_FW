/**
 * \file encoder_calib.h
 * \brief Calibration layer on top of encoder_egtm: maps incremental counts to
 *        an absolute ELECTRICAL rotor angle for FOC.
 *
 * Layering — who owns what:
 *
 *   +--------------------------------------------------------------+
 *   |  FOC current loop (your code)                                 |
 *   |     theta_e = EncoderCalib_getElecAngle()  -> Park/invPark    |
 *   +--------------------------------------------------------------+
 *   |  encoder_calib.c  (THIS module)                               |
 *   |     - EncoderCalibration struct (NVM image, CRC)              |
 *   |     - counts -> electrical angle mapping                      |
 *   |     - reference tracking: alignment ref -> Z-anchored ref     |
 *   |     - alignment state machine (uses your inverter via hooks)  |
 *   +--------------------------------------------------------------+
 *   |  encoder_egtm.c   (unchanged driver)                          |
 *   |     - hardware x4 counting (SPE0_REV_CNT)                     |
 *   |     - Z latch: Encoder_Egtm_service()/getPositionAtZ()        |
 *   +--------------------------------------------------------------+
 *
 * Angle representation: uint32_t, full turn = 2^32 ("u32 angle"),
 * 0 = electrical 0 = rotor d-axis aligned with the phase-U MMF axis.
 * This wraps for free and converts to whatever your trig uses:
 *   radians = angle * (2*pi / 2^32);  q15 sin table index = angle >> 17; etc.
 */

#ifndef ENCODER_CALIB_H
#define ENCODER_CALIB_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ENCODER_CALIB_MAGIC   0x45435631u   /* "ECV1" */

typedef struct
{
    uint32_t magic; /* ENCODER_CALIB_MAGIC                         */
    int32_t zToDAxisCounts; /* counts from Z pulse to electrical 0, i.e.
     theta_e(count) = 0 when
     (count - count_at_Z + zToDAxisCounts) == 0
     (mod CPR/polePairs). Range 0 .. CPR-1.      */
    int8_t direction; /* +1 / -1. NOTE: prefer fixing the sign in
     hardware terms via ENC_INVERT_DIRECTION and
     keeping +1 here — one place for the sign.   */
    uint8_t polePairs; /* p                                           */
    uint8_t reserved[2];
    uint32_t crc; /* CRC-32 (IEEE) over all preceding bytes      */
} EncoderCalibration;

// Saved Configurations
extern const EncoderCalibration default_cal; // Declaration only

/*==============================================================================
 * Runtime API (call from the SAME context as the encoder driver,
 * i.e. your current-loop / control task)
 *============================================================================*/

/**
 * \brief Install calibration data (typically loaded from DFLASH at boot).
 * \return false if magic/CRC/fields are invalid — module stays in
 *         "uncalibrated" state and getElecAngle() reports invalid.
 */
bool EncoderCalib_setData (const EncoderCalibration *cal);

//Main Calibration Process
void calibrationProcess (void);

/** \brief Fill \p cal with the current data + fresh CRC (for storing to NVM
 *         after an alignment run). */
void EncoderCalib_getData (EncoderCalibration *cal);

/**
 * \brief One call per control period. Internally runs
 *        Encoder_Egtm_service() and, when a Z pulse is seen, re-anchors the
 *        electrical angle reference to Z using zToDAxisCounts.
 */
bool EncoderCalib_service (void);

/**
 * \brief Absolute electrical rotor angle, u32 format (2^32 = one electrical
 *        revolution). \param valid true once a reference exists (after
 *        alignment or after the first Z with stored calibration).
 */
uint32_t EncoderCalib_getElecAngle (bool *valid);

/** \brief Electrical angular velocity in u32-angle per call period is simply
 *         the delta of getElecAngle(); helper in float rad/s: */
float EncoderCalib_getElecSpeed (float periodSeconds);

/*==============================================================================
 * One-time / power-up alignment (DC alignment method)
 *
 * The state machine needs to command a stationary current vector through
 * YOUR inverter. Provide these two hooks:
 *============================================================================*/

typedef struct
{
    /** Apply a current (or voltage) vector at electrical angle \p angleU32
     *  with magnitude \p ratio in [0..1] of the configured alignment
     *  current. ratio ramps 0->1 during the alignment. Implement as:
     *  Id* = ratio * I_align, Iq* = 0, with the Park transforms fed the
     *  FORCED angle \p angleU32 (not the encoder angle!). */
    void (*applyVector) (float duty);

    /** Release the inverter (Id*=Iq*=0 / PWM off). */
    void (*releaseVector) (void);

    float rampTimeS; /* current ramp duration, e.g. 1.0f            */
    float settleTimeS; /* standstill wait after full current, 0.5f    */
    float standstillRadS; /* |speed| threshold to accept settling        */
    uint8_t polePairs; /* motor pole pairs (copied into calibration)  */
} EncoderAlignConfig;

typedef enum
{
    ENC_ALIGN_IDLE = 0, ENC_ALIGN_RAMP, /* ramping vector at theta_e = 0                */
    ENC_ALIGN_SETTLE, /* waiting for standstill                       */
    ENC_ALIGN_CAPTURE, /* reference captured, waiting for optional Z   */
    ENC_ALIGN_DONE, /* reference valid; zToDAxisCounts updated if a
     Z pulse was observed                         */
    ENC_ALIGN_FAILED
} EncoderAlignState;

/** \brief Start the alignment. Motor must be free to move (small twitch). */
void EncoderCalib_startAlignment (const EncoderAlignConfig *cfg);

/**
 * \brief Drive the alignment state machine; call once per control period
 *        (same period used for \p periodSeconds).
 * \return current state. On ENC_ALIGN_CAPTURE the drive may already run
 *         closed-loop FOC (reference is valid); keep calling service() and,
 *         if the shaft later passes Z, zToDAxisCounts is measured and the
 *         state advances to DONE — then read EncoderCalib_getData() and
 *         store it to NVM.
 */
EncoderAlignState EncoderCalib_alignmentTask (float periodSeconds);

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_CALIB_H */
