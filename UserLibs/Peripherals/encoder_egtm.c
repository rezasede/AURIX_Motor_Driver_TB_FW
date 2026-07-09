/**
 * \file encoder_egtm.c
 * \brief AURIX TC4Dx hardware quadrature decoder: eGTM TIM0 + SPE0.
 *
 * Every address, offset, bit position and encoding below was taken from the
 * AURIX TC4Dx User Manual v1.1 (2025-06-26), chapter 44 (eGTM). References
 * are given as [UM: register] next to each definition.
 *
 * The driver uses direct register access on purpose: it has zero iLLD
 * dependencies except the End-of-Init unlock for the CLC write, which is
 * isolated behind two macros you can map to your BSP.
 */

#include "encoder_egtm.h"

/*==============================================================================
 * Module enable — TC4x iLLD path (default) or raw CLC write.
 *
 * The TC4x iLLD has no IfxScuWdt.h (watchdog/endinit moved to the WTU).
 * Default: let the iLLD enable the module — IfxEgtm_enable() performs the
 * endinit-protected CLC write internally (same call the official Infineon
 * EGTM_Incremental_Encoder example uses).
 * Define ENC_RAW_CLC_ENABLE to keep this driver iLLD-free; then map the two
 * endinit macros to your BSP (e.g. IfxWtu_clearCpuEndinit/setCpuEndinit with
 * IfxWtu_getCpuWatchdogPassword() from IfxWtu.h).
 *============================================================================*/
#ifndef ENC_RAW_CLC_ENABLE
#include "IfxEgtm.h"
#else
  #ifndef ENC_ENDINIT_CLEAR
    #include "IfxWtu.h"
    #define ENC_ENDINIT_CLEAR()  IfxWtu_clearCpuEndinit(IfxWtu_getCpuWatchdogPassword())
    #define ENC_ENDINIT_SET()    IfxWtu_setCpuEndinit(IfxWtu_getCpuWatchdogPassword())
  #endif
#endif

/*==============================================================================
 * eGTM register map (verified)
 *============================================================================*/

/* [UM: Table 2367 "Registers address space - EGTM"] */
#define EGTM_BASE               0xF9080000u

#define REG32(off)              (*(volatile uint32_t *)(EGTM_BASE + (off)))

/* ---- Infineon wrapper ---------------------------------------------------- */
#define OFF_CLC                 0x060000u   /* [UM: CLC] reset 0x3, DISR bit0 */
#define OFF_TIM0INSEL           0x061000u   /* [UM: TIMnINSEL, 0x061000+n*4]  */
/* CHxSEL nibble at bits 4x+3:4x  */

/* ---- Cluster 0 (stride between clusters would be +i*0x20000) ------------- */
#define OFF_ARCH_CTRL           0x000008u   /* [UM: CLSi_ARCH_CTRL] RF_PROT b0*/
#define OFF_ARCH_CLK_CFG        0x00002Cu   /* [UM: CLSi_ARCH_CLK_CFG]        */
#define OFF_CMU_CLK_EN          0x000080u   /* [UM: CLSi_CMU_CLK_EN]          */
#define OFF_CMU_GCLK_NUM        0x000084u   /* reset 1                        */
#define OFF_CMU_GCLK_DEN        0x000088u   /* reset 1                        */
#define OFF_CMU_CLK0_CTRL       0x00008Cu   /* [UM: CLSi_CMU_CLKx_CTRL x=0]   */
#define OFF_TBU_CHEN            0x000100u   /* [UM: CLSi_TBU_CHEN]            */
#define OFF_TBU_CH0_CTRL        0x000104u   /* [UM: CLSi_TBU_CH0_CTRL]        */
#define OFF_TBU_CH0_BASE        0x000108u   /* [UM: CLSi_TBU_CH0_BASE]        */
#define OFF_CCM_CFG             0x0041F8u   /* [UM: CLSi_CCM_CFG] reset 0x20007 */
#define OFF_CCM_PROT            0x0041FCu   /* [UM: CLSi_CCM_PROT] CLS_PROT b0 */

/* ---- TIM0 channel x (x = 0..7) ------------------------------------------- */
/* [UM: CLSi_TIM_CHx_* register block, base 0x800 + x*0x80]                   */
#define TIM_CH_BASE(x)          (0x000800u + (uint32_t)(x) * 0x80u)
#define OFF_TIM_GPR0(x)         (TIM_CH_BASE(x) + 0x00u)
#define OFF_TIM_GPR1(x)         (TIM_CH_BASE(x) + 0x04u)
#define OFF_TIM_CNT(x)          (TIM_CH_BASE(x) + 0x08u)
#define OFF_TIM_ECNT(x)         (TIM_CH_BASE(x) + 0x0Cu)
#define OFF_TIM_FLT_RE(x)       (TIM_CH_BASE(x) + 0x1Cu)
#define OFF_TIM_FLT_FE(x)       (TIM_CH_BASE(x) + 0x20u)
#define OFF_TIM_CTRL(x)         (TIM_CH_BASE(x) + 0x24u)
#define OFF_TIM_IRQ_NOTIFY(x)   (TIM_CH_BASE(x) + 0x2Cu)
#define OFF_TIM_IRQ_EN(x)       (TIM_CH_BASE(x) + 0x30u)

/* [UM: CLSi_TIM_CHx_CTRL bit fields] */
#define TIM_CTRL_TIM_EN         (1u << 0)
#define TIM_CTRL_TIM_MODE_TIEM  (2u << 1)    /* 010B = Input Event Mode      */
#define TIM_CTRL_DSL_RISING     (1u << 13)   /* rising edge active           */
#define TIM_CTRL_ISL_BOTH       (1u << 14)   /* TIEM: both edges active      */
#define TIM_CTRL_FLT_EN         (1u << 16)
#define TIM_CTRL_FLT_CNT_FRQ0   (0u << 17)   /* filter tick = CMU_CLK_RES0   */
/* FLT_MODE_RE/FE = 0, FLT_CTR_RE/FE = 0 -> immediate edge propagation mode  */
/* GPR0_SEL = 00B (+EGPR0_SEL=0) -> GPR0 captures TIM_TBU_TS0 on each edge   */

/* [UM: CLSi_TIM_CHx_IRQ_EN] */
#define TIM_IRQ_NEWVAL_EN       (1u << 0)

/* ---- SPE0 ----------------------------------------------------------------- */
/* [UM: CLSi_SPE_* register block, SPE of cluster 0]                           */
#define OFF_SPE_CTRL_STAT       0x004C00u
#define OFF_SPE_PAT             0x004C04u
#define OFF_SPE_IRQ_NOTIFY      0x004C2Cu
#define OFF_SPE_IRQ_EN          0x004C30u
#define OFF_SPE_REV_CNT         0x004C40u
#define OFF_SPE_REV_CMP         0x004C44u

/* Bit position of the REV_CNT field inside the register.
 * UM v1.1 documents bits [23:0], but a position quantized in steps of 256
 * indicates the field actually sits at bits [31:8] on the silicon. The iLLD
 * register definitions are generated from the silicon SFR database, so if
 * available, take the truth from there (macro pattern as in IfxEgtm_Spe.h,
 * e.g. IFX_EGTM_CLS_SPE_CTRL_STAT_SIE0_OFF). Otherwise set it manually:
 * observe address 0xF9084C40 in the debugger memory view while turning the
 * shaft one detent -> steps of 0x001 mean shift 0, steps of 0x100 mean 8. */
#ifndef ENC_REV_CNT_SHIFT
#ifdef IFX_EGTM_CLS_SPE_REV_CNT_REV_CNT_OFF
#define ENC_REV_CNT_SHIFT   IFX_EGTM_CLS_SPE_REV_CNT_REV_CNT_OFF
#else
    #define ENC_REV_CNT_SHIFT   8u /* Shift by 8 to align bits [31:8] back to [23:0] */
  #endif
#endif

/* [UM: CLSi_SPE_CTRL_STAT bit fields] */
#define SPE_EN                  (1u << 0)
#define SPE_SIE0                (1u << 1)    /* input x = TIM0 CH0 enable    */
#define SPE_SIE1                (1u << 2)    /* input y = TIM0 CH1 (ENC_B)   */
#define SPE_SIE2                (1u << 3)    /* input z = TIM0 CH2 (ENC_A)   */
#define SPE_TIM_SEL_CLS0        (0u << 6)    /* take TIM of cluster 0        */
#define SPE_PAT_PTR_SHIFT       8            /* 3 bit                        */
#define SPE_AIP_SHIFT           12           /* 3 bit, current pattern       */
#define SPE_ADIR                (1u << 15)
#define SPE_PIP_SHIFT           16           /* 3 bit, previous pattern      */
#define SPE_NIP_SHIFT           20           /* 3 bit, live pattern (rh)     */

/* [UM: CLSi_SPE_IRQ_NOTIFY bit fields] (rw1c) */
#define SPE_IRQ_NIPD            (1u << 0)
#define SPE_IRQ_DCHG            (1u << 1)
#define SPE_IRQ_PERR            (1u << 2)
#define SPE_IRQ_BIS             (1u << 3)
#define SPE_IRQ_RCMP            (1u << 4)

/*==============================================================================
 * Quadrature pattern table for CLS0_SPE_PAT
 *
 * [UM: CLSi_SPE_PAT] nibble t (bits 4t+3 : 4t):
 *      bit0 = IPt_VAL, bit1 = TIM0 CH0 (masked, always 0),
 *      bit2 = TIM0 CH1 = ENC_B, bit3 = TIM0 CH2 = ENC_A
 *
 * 4-state Gray sequence (A,B): 00 -> 10 -> 11 -> 01 -> ... programmed TWICE,
 * so the 3-bit SPE_PAT_PTR (mod 8) always sees a consistent neighbourhood:
 *
 *   t:        0     1     2     3     4     5     6     7
 *   (A,B):   0,0   1,0   1,1   0,1   0,0   1,0   1,1   0,1
 *   nibble:  0x1   0x9   0xD   0x5   0x1   0x9   0xD   0x5
 *
 * Forward (REV_CNT incrementing) = A leads B. ENC_INVERT_DIRECTION reverses
 * the sequence instead (B leads A), which flips the counting direction.
 *============================================================================*/
#if ENC_INVERT_DIRECTION
  #define SPE_PAT_VALUE   0x9D519D51u   /* 00 -> 01 -> 11 -> 10 -> ...       */
  /* pattern (A<<2|B<<1) -> table index for the inverted sequence            */
  static const uint8_t patToIdx[4] = { 0u /*00*/, 1u /*01*/, 3u /*10*/, 2u /*11*/ };
#else
#define SPE_PAT_VALUE   0x5D915D91u   /* 00 -> 10 -> 11 -> 01 -> ...       */
static const uint8_t patToIdx[4] = {0u /*00*/, 3u /*01*/, 1u /*10*/, 2u /*11*/};
#endif

/* SPE 3-bit pattern value for a given table index (matches SPE_PAT_VALUE)   */
static uint8_t idxToPat (uint8_t idx)
{
    return (uint8_t) ((SPE_PAT_VALUE >> (4u * (idx & 3u) + 1u)) & 0x7u);
}

/*==============================================================================
 * Channel assignment (fixed by pin routing, verified twice:
 * iLLD pin map AND [UM: EGTM_TIMnINSEL CHxSEL value tables]:
 *   TIM0INSEL.CH1SEL = 1H -> P00.10   (ENC_B)
 *   TIM0INSEL.CH2SEL = 1H -> P00.11   (ENC_A)
 *   TIM0INSEL.CH3SEL = 1H -> P00.12   (ENC_Z)
 *============================================================================*/
#define CH_B    1u
#define CH_A    2u
#define CH_Z    3u

/*==============================================================================
 * Private state
 *============================================================================*/
static struct
{
    uint32_t lastRevCntRaw; /* last 24-bit hardware value                  */
    int32_t positionAcc; /* 32-bit software-extended position           */
    int32_t zeroOffset; /* software zero for reset()                   */
    uint32_t zCntOffset; /* software zero for the Z pulse counter       */
    int32_t lastSpeedPos;
    uint32_t stickyErrors;
    uint32_t lastZCntSeen; /* CH3 CNT value at the last detected Z        */
    int32_t positionAtZ; /* position latched at the last Z pulse        */
    bool zSeen; /* at least one Z since init                   */
    bool initialised;
} g;

/*==============================================================================
 * Helpers
 *============================================================================*/

static inline int32_t signExtend24 (uint32_t v)
{
    return ((int32_t) (v << 8)) >> 8;
}

//static inline int32_t signExtend24(uint32_t v)
//{
//    /* * Pure mathematical 24-bit sign extension.
//     * This avoids ALL compiler bugs related to arithmetic vs. logical bit-shifting
//     * and unsigned-to-signed casting quirks.
//     */
//    if (v >= 0x00800000u)
//    {
//        /* e.g. Underflow to 0xFFFFFF (16,777,215) -> 16,777,215 - 16,777,216 = -1 */
//        return (int32_t)v - 16777216;
//    }
//
//    /* Positive numbers (0 to 8,388,607) return exactly as they are */
//    return (int32_t)v;
//}

/* Read REV_CNT and fold the delta into the 32-bit accumulator. */
static int32_t updatePosition (void)
{
    uint32_t raw = (REG32(OFF_SPE_REV_CNT) >> ENC_REV_CNT_SHIFT) & 0x00FFFFFFu;
    int32_t delta = signExtend24((raw - g.lastRevCntRaw) & 0x00FFFFFFu);
    g.lastRevCntRaw = raw;
    g.positionAcc += delta;
    return g.positionAcc;
}

static void configTimChannel (uint32_t ch, uint32_t edgeCfg)
{
    /* Order per [UM]: FLT_* only take effect with FLT_EN; TIM_MODE/DSL/ISL
     * must not be changed while the channel is enabled -> configure first,
     * then set TIM_EN with the final value. Enabling resets ECNT/CNT/GPRx. */
    REG32(OFF_TIM_FLT_RE(ch)) = ENC_TIM_FILTER_TICKS;
    REG32(OFF_TIM_FLT_FE(ch)) = ENC_TIM_FILTER_TICKS;

    uint32_t ctrl = TIM_CTRL_TIM_MODE_TIEM | edgeCfg | TIM_CTRL_FLT_EN | TIM_CTRL_FLT_CNT_FRQ0;
    /* GPR0_SEL=0 -> TBU_TS0 timestamp captured on each edge   */

    REG32(OFF_TIM_CTRL(ch)) = ctrl;
    REG32(OFF_TIM_CTRL(ch)) = ctrl | TIM_CTRL_TIM_EN;
}

/*==============================================================================
 * Public API
 *============================================================================*/

bool Encoder_Egtm_init (void)
{
    /* --- 1. Enable the eGTM module ---------------------------------------- */
#ifndef ENC_RAW_CLC_ENABLE
    if (!IfxEgtm_isEnabled(&MODULE_EGTM))
    {
        IfxEgtm_enable(&MODULE_EGTM); /* endinit handled inside     */
    }
#else
    ENC_ENDINIT_CLEAR();
    REG32(OFF_CLC) = 0u;                       /* CLC.DISR = 0               */
    ENC_ENDINIT_SET();
#endif
    if ((REG32(OFF_CLC) & 0x2u) != 0u) /* DISS still set -> no clock */
    {
        return false;
    }

    /* --- 2. Cluster / CMU clocks ------------------------------------------
     * Reset defaults already enable all clusters (divider /2) and the module
     * sub-block clocks (CCM_CFG reset = EN_TIM|EN_TOM_SPE_TDTM|EN_ATOM).
     * We only need CMU_CLK_RES0 running for the TIM glitch filters:
     * GCLK_NUM = GCLK_DEN = 1 (reset default), CLK0 divider = 0, enable CLK0.
     * [UM: CLSi_CMU_CLK_EN] write 10B to an EN_CLKx field to enable.        */
    REG32(OFF_CMU_GCLK_NUM) = 1u;
    REG32(OFF_CMU_GCLK_DEN) = 1u;
    REG32(OFF_CMU_CLK0_CTRL) = 0u; /* CLK_RES0 = cluster clk */
    REG32(OFF_CMU_CLK_EN) = (2u << 0); /* EN_CLK0 = 10B          */

    /* --- 3. TBU channel 0 free-running (timestamp base for Z capture) ----- */
    REG32(OFF_TBU_CH0_BASE) = 0u;
    REG32(OFF_TBU_CH0_CTRL) = 0u; /* CLK_RES0, low res      */
    REG32(OFF_TBU_CHEN) = (2u << 0); /* ENDIS_CH0 = 10B enable */

    /* --- 4. Route the pins into TIM0 [UM: EGTM_TIMnINSEL] ------------------
     * CH1SEL=1 (P00.10/B), CH2SEL=1 (P00.11/A), CH3SEL=1 (P00.12/Z).        */
    {
        uint32_t insel = REG32(OFF_TIM0INSEL);
        insel &= ~((0xFu << 4) | (0xFu << 8) | (0xFu << 12));
        insel |= ((0x1u << 4) | (0x1u << 8) | (0x1u << 12));
        REG32(OFF_TIM0INSEL) = insel;
    }

    /* --- 5. TIM0 channels ---------------------------------------------------
     * A/B: TIEM, both edges (keeps CNT/ECNT usable as diagnostics; the SPE
     *      taps the FILTERED level F_OUT regardless of the mode).
     * Z  : TIEM, rising edge only -> CNT counts index pulses in hardware,
     *      GPR0 captures the TBU_TS0 timestamp of each index pulse.          */
    configTimChannel(CH_B, TIM_CTRL_ISL_BOTH);
    configTimChannel(CH_A, TIM_CTRL_ISL_BOTH);
    configTimChannel(CH_Z, TIM_CTRL_DSL_RISING); /* ISL=0, DSL=1           */

    /* --- 6. SPE0: pattern table, state pre-load, enable ---------------------
     * [UM: SPE chapter] Before enabling, read NIP and initialise AIP, PIP and
     * SPE_PAT_PTR consistently. Input x (TIM0 CH0) is masked with SIE0=0 and
     * is sampled as constant 0.                                              */
    REG32(OFF_SPE_PAT) = SPE_PAT_VALUE;

    {
        uint32_t nip = (REG32(OFF_SPE_CTRL_STAT) >> SPE_NIP_SHIFT) & 0x7u;
        uint32_t ab = (nip >> 1) & 0x3u; /* strip masked CH0 bit,
         leaves (A<<1)|B        */
        /* patToIdx is indexed with (A<<2|B<<1)>>1 = (A<<1|B): */
        uint8_t idx = patToIdx[ab];
        uint8_t cur = idxToPat(idx);
        uint8_t prev = idxToPat((uint8_t) ((idx + 3u) & 3u));

        uint32_t cfg = SPE_SIE1 | SPE_SIE2 | SPE_TIM_SEL_CLS0 | ((uint32_t) idx << SPE_PAT_PTR_SHIFT)
                | ((uint32_t) cur << SPE_AIP_SHIFT) | ((uint32_t) prev << SPE_PIP_SHIFT);

        REG32(OFF_SPE_CTRL_STAT) = cfg; /* configure...           */
        REG32(OFF_SPE_REV_CNT) = 0u; /* known start value      */
        REG32(OFF_SPE_IRQ_NOTIFY) = SPE_IRQ_NIPD | SPE_IRQ_DCHG | SPE_IRQ_PERR | SPE_IRQ_BIS | SPE_IRQ_RCMP; /* clear    */
        REG32(OFF_SPE_CTRL_STAT) = cfg | SPE_EN; /* ...then enable         */
    }

    g.lastRevCntRaw = 0u;
    g.positionAcc = 0;
    g.zeroOffset = 0;
    g.zCntOffset = 0u;
    g.lastSpeedPos = 0;
    g.stickyErrors = 0u;
    g.lastZCntSeen = 0u;
    g.positionAtZ = 0;
    g.zSeen = false;
    g.initialised = true;
    return true;
}

int32_t Encoder_Egtm_getPosition (void)
{
    return updatePosition() - g.zeroOffset;
}

float Encoder_Egtm_getAngleRad (void)
{
    const float twoPi = 6.283185307179586f;
    int32_t p = Encoder_Egtm_getPosition();
    int32_t m = p % (int32_t) ENC_COUNTS_PER_REV;
    if (m < 0)
    {
        m += (int32_t) ENC_COUNTS_PER_REV;
    }
    return ((float) m / (float) ENC_COUNTS_PER_REV) * twoPi;
}

void Encoder_Egtm_getStatus (Encoder_Egtm_Status *status)
{
    uint32_t irq = REG32(OFF_SPE_IRQ_NOTIFY);
    uint32_t err = 0u;
    if (irq & SPE_IRQ_PERR)
    {
        err |= 1u;
    }
    if (irq & SPE_IRQ_BIS)
    {
        err |= 2u;
    }
    g.stickyErrors |= err;

    status->position = Encoder_Egtm_getPosition();
    status->revolutions = (int32_t) ((REG32(OFF_TIM_CNT(CH_Z)) - g.zCntOffset) & 0x00FFFFFFu);
    status->zTimestamp = REG32(OFF_TIM_GPR0(CH_Z)) & 0x00FFFFFFu;
    status->direction = ((REG32(OFF_SPE_CTRL_STAT) & SPE_ADIR) != 0u);
    status->errorFlags = g.stickyErrors;
}

void Encoder_Egtm_reset (void)
{
    g.zeroOffset += Encoder_Egtm_getPosition(); /* pure software rebase   */
    g.zCntOffset = REG32(OFF_TIM_CNT(CH_Z)) & 0x00FFFFFFu;
    g.lastSpeedPos = 0;
}

void Encoder_Egtm_clearErrors (void)
{
    REG32(OFF_SPE_IRQ_NOTIFY) = SPE_IRQ_PERR | SPE_IRQ_BIS; /* rw1c        */
    g.stickyErrors = 0u;
}

float Encoder_Egtm_updateSpeed (float periodSeconds)
{
    const float twoPi = 6.283185307179586f;
    int32_t pos = Encoder_Egtm_getPosition();
    int32_t delta = pos - g.lastSpeedPos;
    g.lastSpeedPos = pos;
    return ((float) delta / (float) ENC_COUNTS_PER_REV) * twoPi / periodSeconds;
}

void Encoder_Egtm_enableZInterrupt (void)
{
    /* One TIM_NEWVAL interrupt per index pulse. The eGTM-internal enable is
     * done here; connecting the eGTM service request to a CPU/priority in
     * the Interrupt Router is application-specific (see README).            */
    REG32(OFF_TIM_IRQ_EN(CH_Z)) = TIM_IRQ_NEWVAL_EN;
}

/*==============================================================================
 * Z-latch service (hook for the calibration layer)
 *============================================================================*/

bool Encoder_Egtm_service (void)
{
    uint32_t zCnt = REG32(OFF_TIM_CNT(CH_Z)) & 0x00FFFFFFu;
    if (zCnt != g.lastZCntSeen)
    {
        g.lastZCntSeen = zCnt;
        g.positionAtZ = Encoder_Egtm_getPosition();
        g.zSeen = true;
        return true;
    }
    return false;
}

void Encoder_Egtm_notifyZ (void)
{
    g.lastZCntSeen = REG32(OFF_TIM_CNT(CH_Z)) & 0x00FFFFFFu;
    g.positionAtZ = Encoder_Egtm_getPosition();
    g.zSeen = true;
}

int32_t Encoder_Egtm_getPositionAtZ (bool *valid)
{
    if (valid != (void*) 0)
    {
        *valid = g.zSeen;
    }
    return g.positionAtZ;
}
