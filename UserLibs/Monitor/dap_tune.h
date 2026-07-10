/**********************************************************************************************************************
 * \file    dap_tune.h
 * \brief   Live FOC parameter tuning over DAP/DAS (OneEye), validated on CPU1.
 *
 * PRINCIPLE
 * ---------
 * DAS lets OneEye read/write MCU RAM directly over DAP while the CPUs run. No target-side protocol is
 * needed; OneEye resolves variable addresses from the .elf file. The problem is coherency: OneEye writes
 * ONE 32-bit word at a time, asynchronously to the FOC ISR. A new Kp with an old Ki must never reach the
 * controller. Therefore:
 *
 *   OneEye (DAP) --> g_dapTune.edit        (staging, GUI writes freely, field by field)
 *                    g_dapTune.apply = 1   (GUI sets this LAST, after editing)
 *   CPU1, 1 ms   --> DapTune_task()        (sees apply, clamps to limits, publishes under seqlock)
 *   FOC core ISR --> DapTune_read(&p)      (returns true once per accepted set, always a coherent copy)
 *
 * PLACEMENT
 * ---------
 * g_dapTune is pinned to section ".bss_cpu1" -> DSPR1 (0x6000_0000, see Lcf_Gcc_Tricore_Tc.lsl).
 * Scratchpad RAM is never cached, is visible to all cores over the SRI, and to DAS. So there is no
 * cache-coherency problem by construction. All shared fields are volatile; aligned 32-bit accesses
 * are atomic on the SRI. Keep every field 32 bit (float/uint32_t) — no doubles.
 *
 * HOOK-UP (3 places)
 * ------------------
 *   1) FOC core, once, after gains are computed:
 *          DapTune_Params p0 = { kpD0, kiD0, kpQ0, kiQ0, 0.0f, 0.0f, 0u };
 *          DapTune_init(&p0);
 *
 *   2) Cpu1_Main.c, 1 ms background loop:
 *          DapTune_task();
 *
 *   3) FOC ISR (or its 1 kHz slow task), every cycle:
 *          DapTune_Params p;
 *          if (DapTune_read(&p))
 *          {
 *              -- apply p.kpD/kiD/kpQ/kiQ to the PI controllers
 *              -- apply p.idRef_A/p.iqRef_A via Foc_setCurrentSetpoints()
 *              -- gate the drive with p.enable
 *          }
 *      Telemetry out (optional, plain stores, no locking needed):
 *          g_dapTune.monitor.idMeas_A = ...; g_dapTune.monitor.iqMeas_A = ...;
 *
 * ONEEYE SIDE
 * -----------
 * Debug box -> filter "g_dapTune" -> Add watch. Create signals for edit.* fields and for "apply".
 * Workflow in the GUI: type new values into edit.*, then set apply = 1. CPU1 clamps, publishes,
 * writes the ACCEPTED values back into edit.* and clears apply. applyCount increments once per
 * accepted set; clamped tells how many fields were limited on the last apply.
 *
 * EXTENDING
 * ---------
 * Add a 32-bit field to DapTune_Params, add one clamp line in dapTune_validate(), rebuild.
 * OneEye picks the new field up from the ELF automatically.
 *********************************************************************************************************************/

#ifndef DAP_TUNE_H_
#define DAP_TUNE_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
/* Safety limits applied by CPU1 before a set is published. Adjust to your machine.
 * They exist so a fat-fingered GUI entry (400 instead of 4.00) can never reach the current loop. */
#define DAPTUNE_KP_MIN          (0.0f)      /* [V/A]                                    */
#define DAPTUNE_KP_MAX          (100.0f)    /* [V/A]                                    */
#define DAPTUNE_KI_MIN          (0.0f)      /* [V/(A*s)]                                */
#define DAPTUNE_KI_MAX          (50000.0f)  /* [V/(A*s)]                                */
#define DAPTUNE_IREF_MAX_A      (30.0f)      /* [A] |id|,|iq| limit for bring-up         */

/*********************************************************************************************************************/
/*-------------------------------------------------Data Structures---------------------------------------------------*/
/*********************************************************************************************************************/
/* One coherent parameter set. 32-bit fields only (single DAP access per field). */
typedef struct
{
    float kpD; /* d-axis PI proportional gain  [V/A]      */
    float kiD; /* d-axis PI integral gain      [V/(A*s)]  */
    float kpQ; /* q-axis PI proportional gain  [V/A]      */
    float kiQ; /* q-axis PI integral gain      [V/(A*s)]  */
    float idRef_A; /* d current setpoint           [A]        */
    float iqRef_A; /* q current setpoint           [A]        */
    uint32_t enable; /* 0 = drive gated off, 1 = run            */
} DapTune_Params;

/* Live values written by the FOC ISR for plotting in OneEye. Single writer (the ISR),
 * word-atomic reads by DAS: no locking needed for scope-style monitoring. */
typedef struct
{
    float idMeas_A; /* measured id [A]                          */
    float iqMeas_A; /* measured iq [A]                          */
    uint32_t elecAngleRaw; /* electrical angle, u32 fixed-point format */
    float aux; /* free channel                             */
} DapTune_Monitor;

/* Everything OneEye touches, in one struct at one address. */
typedef struct
{
    uint32_t magic; /* DAPTUNE_MAGIC once DapTune_init() ran; gates task and read  */
    uint32_t apply; /* GUI: set to 1 after editing 'edit'; CPU1 clears it          */
    uint32_t applyCount; /* incremented by CPU1 once per accepted set                   */
    uint32_t clamped; /* number of fields limited on the last apply (GUI feedback)   */
    uint32_t seq; /* seqlock: odd = 'active' being written, even = stable        */
    DapTune_Params edit; /* staging: written by OneEye over DAP                         */
    DapTune_Params active; /* published: written only by CPU1, read by the FOC core       */
    DapTune_Monitor monitor; /* telemetry: written by the FOC ISR, read by OneEye           */
} DapTune_Shared;

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
extern volatile DapTune_Shared g_dapTune;

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
/* Call once on the FOC core with the computed default gains, before the loops are trusted.
 * Seeds edit and active with 'initial' and arms the module (writes magic last). */
void DapTune_init (const DapTune_Params *initial);

/* Call periodically on CPU1 (1 ms is fine). Consumes 'apply': clamp -> publish -> acknowledge.
 * Does nothing until DapTune_init() ran. */
void DapTune_task (void);

/* Call from the FOC ISR (or its slow task) on ONE core only. Copies a coherent snapshot into *dst
 * and returns true exactly when a newly accepted set is available; returns false otherwise
 * (keep using your current parameters then). Bounded, lock-free, no waiting. */
bool DapTune_read (DapTune_Params *dst);

#endif /* DAP_TUNE_H_ */
