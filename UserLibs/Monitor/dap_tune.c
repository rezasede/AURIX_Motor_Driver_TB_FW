/**********************************************************************************************************************
 * \file    dap_tune.c
 * \brief   Live FOC parameter tuning over DAP/DAS (OneEye). See dap_tune.h for the design.
 *
 * Concurrency model, exact and short:
 *   - OneEye (DAP master) writes edit.* and apply. Aligned 32-bit SRI accesses are atomic.
 *   - CPU1 (DapTune_task) is the ONLY writer of active, seq, applyCount, clamped.
 *   - The FOC core (DapTune_read) only reads. The seqlock (seq odd/even) guarantees the copy of
 *     'active' is coherent; volatile keeps the compiler from reordering the accesses.
 *   - No core ever spins on another. Worst case DapTune_read returns false and the controller
 *     simply keeps its current parameters for one more cycle.
 *********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "dap_tune.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define DAPTUNE_MAGIC           (0x54554E45u)   /* "TUNE" */
#define DAPTUNE_READ_RETRIES    (3u)            /* publish lasts ~10 stores; one retry already generous */

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
/* Pinned to DSPR1 (see Lcf_Gcc_Tricore_Tc.lsl: "*(.bss_cpu1)" -> dsram1 @ 0x6000_0000).
 * Scratchpad RAM: never cached, visible to all cores and to DAS. */
volatile DapTune_Shared g_dapTune __attribute__((section(".bss_cpu1"), aligned(4)));

/*********************************************************************************************************************/
/*--------------------------------------------Private Function Definitions-------------------------------------------*/
/*********************************************************************************************************************/
/* Clamp with NaN guard: NaN compares false to everything, so test (v == v) explicitly.
 * A NaN typed into the GUI becomes 'lo' instead of poisoning the controller. */
static float dapTune_clampf(float v, float lo, float hi, uint32_t *clampedCnt)
{
    float r = v;

    if (!(v == v))          { r = lo; }
    else if (v < lo)        { r = lo; }
    else if (v > hi)        { r = hi; }

    if (r != v)
    {
        (*clampedCnt)++;
    }
    return r;
}

/* Copy the staged set into *dst (plain RAM) and force every field into its safe range. */
static void dapTune_validate(DapTune_Params *dst, uint32_t *clampedCnt)
{
    /* Field-by-field volatile reads from the staging area */
    dst->kpD     = g_dapTune.edit.kpD;
    dst->kiD     = g_dapTune.edit.kiD;
    dst->kpQ     = g_dapTune.edit.kpQ;
    dst->kiQ     = g_dapTune.edit.kiQ;
    dst->idRef_A = g_dapTune.edit.idRef_A;
    dst->iqRef_A = g_dapTune.edit.iqRef_A;
    dst->enable  = (g_dapTune.edit.enable != 0u) ? 1u : 0u;

    *clampedCnt = 0u;
    dst->kpD     = dapTune_clampf(dst->kpD,     DAPTUNE_KP_MIN,     DAPTUNE_KP_MAX,    clampedCnt);
    dst->kiD     = dapTune_clampf(dst->kiD,     DAPTUNE_KI_MIN,     DAPTUNE_KI_MAX,    clampedCnt);
    dst->kpQ     = dapTune_clampf(dst->kpQ,     DAPTUNE_KP_MIN,     DAPTUNE_KP_MAX,    clampedCnt);
    dst->kiQ     = dapTune_clampf(dst->kiQ,     DAPTUNE_KI_MIN,     DAPTUNE_KI_MAX,    clampedCnt);
    dst->idRef_A = dapTune_clampf(dst->idRef_A, -DAPTUNE_IREF_MAX_A, DAPTUNE_IREF_MAX_A, clampedCnt);
    dst->iqRef_A = dapTune_clampf(dst->iqRef_A, -DAPTUNE_IREF_MAX_A, DAPTUNE_IREF_MAX_A, clampedCnt);
}

/* Field-by-field volatile stores (memcpy would discard the volatile qualifier). */
static void dapTune_storeParams(volatile DapTune_Params *dst, const DapTune_Params *src)
{
    dst->kpD     = src->kpD;
    dst->kiD     = src->kiD;
    dst->kpQ     = src->kpQ;
    dst->kiQ     = src->kiQ;
    dst->idRef_A = src->idRef_A;
    dst->iqRef_A = src->iqRef_A;
    dst->enable  = src->enable;
}

static void dapTune_loadParams(DapTune_Params *dst, const volatile DapTune_Params *src)
{
    dst->kpD     = src->kpD;
    dst->kiD     = src->kiD;
    dst->kpQ     = src->kpQ;
    dst->kiQ     = src->kiQ;
    dst->idRef_A = src->idRef_A;
    dst->iqRef_A = src->iqRef_A;
    dst->enable  = src->enable;
}

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
void DapTune_init(const DapTune_Params *initial)
{
    g_dapTune.magic      = 0u;              /* disarm while seeding */
    g_dapTune.apply      = 0u;
    g_dapTune.applyCount = 0u;
    g_dapTune.clamped    = 0u;
    g_dapTune.seq        = 0u;

    dapTune_storeParams(&g_dapTune.edit,   initial);
    dapTune_storeParams(&g_dapTune.active, initial);

    g_dapTune.monitor.idMeas_A     = 0.0f;
    g_dapTune.monitor.iqMeas_A     = 0.0f;
    g_dapTune.monitor.elecAngleRaw = 0u;
    g_dapTune.monitor.aux          = 0.0f;

    g_dapTune.magic = DAPTUNE_MAGIC;        /* arm LAST; volatile ordering guarantees this store is final */
}

void DapTune_task(void)
{
    DapTune_Params tmp;
    uint32_t       clampedCnt;

    if (g_dapTune.magic != DAPTUNE_MAGIC)   /* not armed yet: FOC core has not seeded defaults */
    {
        return;
    }

    if (g_dapTune.apply == 0u)              /* nothing requested by the GUI */
    {
        return;
    }

    /* 1) Stage -> local copy, forced into safe ranges */
    dapTune_validate(&tmp, &clampedCnt);

    /* 2) Publish under the seqlock: odd = writing, even = stable */
    g_dapTune.seq = g_dapTune.seq + 1u;     /* -> odd  */
    dapTune_storeParams(&g_dapTune.active, &tmp);
    g_dapTune.seq = g_dapTune.seq + 1u;     /* -> even */

    /* 3) Acknowledge: count the set, report clamps, echo accepted values back to the GUI */
    g_dapTune.applyCount = g_dapTune.applyCount + 1u;
    g_dapTune.clamped    = clampedCnt;
    dapTune_storeParams(&g_dapTune.edit, &tmp);
    g_dapTune.apply      = 0u;
}

bool DapTune_read(DapTune_Params *dst)
{
    static uint32_t lastApplyCount = 0u;    /* single consumer only: the FOC core */
    uint32_t        i;

    if (g_dapTune.magic != DAPTUNE_MAGIC)
    {
        return false;
    }

    /* Read the counter BEFORE the copy: a coherent copy taken afterwards is at least this new.
     * Worst case the same set is reported twice, which is harmless (applying it is idempotent). */
    uint32_t applyCount = g_dapTune.applyCount;
    if (applyCount == lastApplyCount)
    {
        return false;                        /* nothing new, skip the copy entirely */
    }

    for (i = 0u; i < DAPTUNE_READ_RETRIES; i++)
    {
        uint32_t s1 = g_dapTune.seq;
        if ((s1 & 1u) != 0u)
        {
            continue;                        /* CPU1 is publishing right now, try again */
        }

        dapTune_loadParams(dst, &g_dapTune.active);

        if (g_dapTune.seq == s1)             /* nothing changed during the copy -> coherent */
        {
            lastApplyCount = applyCount;
            return true;
        }
    }

    return false;                            /* collided every try: keep old params, retry next cycle */
}
