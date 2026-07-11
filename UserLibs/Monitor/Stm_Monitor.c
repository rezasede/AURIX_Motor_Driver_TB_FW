/**
 * \file Stm_Monitor.c
 * \brief Single STM interrupt on CPU2, period = PWM period.
 *        Sampling is lossless: every tick pushes one sample into the ring
 *        and drains up to MON_MAX_SENDS_PER_TICK samples out as JSON UDP.
 *        All GETH access happens in this one context — no concurrency.
 */
#include "Stm_Monitor.h"
#include "IfxStm.h"
#include "IfxCpu.h"
#include "IfxSrc.h"
#include "Monitor_Json.h"
#include "Eth_Core.h"
#include "Adc_MotorSense.h"
#include "GPIO_LED_Button.h"
#include "Control_Loops.h"      /* PWM_Period, g_foc, theta, speed, CTRL_S */
#include "encoder_calib.h"
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Local configuration                                                 */
/* ------------------------------------------------------------------ */
#define MONITOR_STM     (&MODULE_CPU2)
#define TICKS_PER_MS    ((uint32)(1e-3 / (PWM_Period) + 0.5))  /* 50us -> 20 */
#define UDP_BUF_SIZE    (512u)

/* ------------------------------------------------------------------ */
/* Module data                                                         */
/* ------------------------------------------------------------------ */
static IfxStm_CompareConfig s_stmCfg;
static uint32               s_reloadTicks;
static MonitorData          s_monitorData;
static char                 s_udpBuffer[UDP_BUF_SIZE];
static uint32               s_msgCounter = 0u;   /* "n" — PC-side loss check */

MonRing g_monRing;

extern volatile Adc_PhaseCurrents g_currents;
extern volatile Adc_PhaseVoltages g_voltages;

/* ------------------------------------------------------------------ */
/* Forward declarations                                                */
/* ------------------------------------------------------------------ */
static inline void Mon_push(const MonSample *s);
static void        Mon_drainJson(uint32 maxSends);
#if MON_USE_BINARY_BULK
static void        Mon_drainAndSendBinary(void);
#endif

/* ------------------------------------------------------------------ */
/* ISR                                                                 */
/* ------------------------------------------------------------------ */
IFX_INTERRUPT(stm2MonitorISR, 2, STM_MONITOR_ISR_PRIO)
{
    IfxStm_clearCompareFlag(MONITOR_STM, s_stmCfg.comparator);
    IfxStm_increaseCompare(MONITOR_STM, s_stmCfg.comparator, s_reloadTicks);

    /* ---- every tick: snapshot into ring (never skipped) ---- */
    MonSample s;
    s.t_us   = IfxStm_getMicrosecondsFromTicks(IfxStm_get(MONITOR_STM));
    s.ia     = g_currents.ia;
    s.ib     = g_currents.ib;
    s.ic     = g_currents.ic;
    s.va     = g_voltages.va;
    s.vb     = g_voltages.vb;
    s.vc     = g_voltages.vc;
    s.vdc    = g_voltages.vdc;
    s.id     = g_foc.iDq.d;
    s.iq     = g_foc.iDq.q;
    s.theta  = (float32)theta;
    s.speed  = speed;
    s.status = (uint32)CTRL_S;
    s.AUX1 = (int32_t)AUX1;
    Mon_push(&s);

    /* ---- every tick: drain a little (spread load, lossless) ---- */
    if (Eth_Core2_isLinkUp())
    {
#if MON_USE_BINARY_BULK
        /* binary path drains in the 1 ms slot below instead */
#else
        Mon_drainJson(MON_MAX_SENDS_PER_TICK);
#endif
    }

    /* ---- every 1 ms: Ethernet housekeeping ---- */
    static uint32 msDiv = 0u;
    if (++msDiv >= TICKS_PER_MS)
    {
        msDiv = 0u;

        Eth_Core2_service1ms();

        if (Eth_Core2_isLinkUp())
        {
#if MON_USE_BINARY_BULK
            Mon_drainAndSendBinary();
#endif
        }
        else
        {
            g_monRing.rd = g_monRing.wr;    /* no link -> discard */
        }

        static uint32 secDiv = 0u;
        if (++secDiv >= 1000u)
        {
            secDiv = 0u;
            toggleLed(LED_1s);
        }
    }
}

/* ------------------------------------------------------------------ */
/* JSON path: drain up to maxSends samples, one datagram each          */
/* ------------------------------------------------------------------ */
static void Mon_drainJson(uint32 maxSends)
{
    for (uint32 k = 0u; (k < maxSends) && (g_monRing.rd != g_monRing.wr); k++)
    {
        const MonSample *s = &g_monRing.buf[g_monRing.rd];
        g_monRing.rd = (g_monRing.rd + 1u) & MON_RING_MASK;

        MonitorJson_reset(&s_monitorData, (float64)s->t_us * 1e-6);

        MonitorJson_add(&s_monitorData, "n",     (float32)(s_msgCounter++ & 0xFFFFFFu));
        MonitorJson_add(&s_monitorData, "ia",    s->ia);
        MonitorJson_add(&s_monitorData, "ib",    s->ib);
        MonitorJson_add(&s_monitorData, "ic",    s->ic);
        MonitorJson_add(&s_monitorData, "va",    s->va);
        MonitorJson_add(&s_monitorData, "vb",    s->vb);
        MonitorJson_add(&s_monitorData, "vc",    s->vc);
        MonitorJson_add(&s_monitorData, "vdc",   s->vdc);
        MonitorJson_add(&s_monitorData, "id",    s->id);
        MonitorJson_add(&s_monitorData, "iq",    s->iq);
        MonitorJson_add(&s_monitorData, "theta", s->theta);
        MonitorJson_add(&s_monitorData, "speed", s->speed);
        MonitorJson_add(&s_monitorData, "status",(float32)s->status);

        uint32 len = MonitorJson_build(&s_monitorData, s_udpBuffer, sizeof(s_udpBuffer));
        if (len > 0u)
        {
            Eth_Core2_sendUdp(s_udpBuffer, len);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Binary bulk path (full rate, ~4x less bandwidth, needs PC bridge)   */
/* ------------------------------------------------------------------ */
#if MON_USE_BINARY_BULK

typedef struct
{
    uint32 magic;        /* 0x4D4F4E31 = "MON1"        */
    uint32 seq;          /* packet counter, loss check */
    uint16 count;        /* samples in this packet     */
    uint16 sampleSize;   /* = sizeof(MonSample)        */
} MonPktHeader;

#define SAMPLES_PER_PKT (28u)   /* 12 + 28*52 = 1468 B < MTU */

static uint32 s_seq = 0u;
static uint8  s_pkt[sizeof(MonPktHeader) + SAMPLES_PER_PKT * sizeof(MonSample)];

static void Mon_drainAndSendBinary(void)
{
    while (g_monRing.rd != g_monRing.wr)
    {
        MonPktHeader *h   = (MonPktHeader *)s_pkt;
        MonSample    *dst = (MonSample *)(s_pkt + sizeof(MonPktHeader));
        uint16        n   = 0u;

        while ((n < SAMPLES_PER_PKT) && (g_monRing.rd != g_monRing.wr))
        {
            dst[n++]     = g_monRing.buf[g_monRing.rd];
            g_monRing.rd = (g_monRing.rd + 1u) & MON_RING_MASK;
        }

        h->magic      = 0x4D4F4E31u;
        h->seq        = s_seq++;
        h->count      = n;
        h->sampleSize = (uint16)sizeof(MonSample);

        Eth_Core2_sendUdp((char *)s_pkt,
                          sizeof(MonPktHeader) + n * sizeof(MonSample));
    }
}
#endif /* MON_USE_BINARY_BULK */

/* ------------------------------------------------------------------ */
/* Ring buffer (single producer / single consumer, same core)          */
/* ------------------------------------------------------------------ */
static inline void Mon_push(const MonSample *s)
{
    uint32 wr   = g_monRing.wr;
    uint32 next = (wr + 1u) & MON_RING_MASK;

    if (next == g_monRing.rd)           /* full -> drop, never block */
    {
        g_monRing.overrun++;
        return;
    }
    g_monRing.buf[wr] = *s;
    __dsync();                          /* data lands before index   */
    g_monRing.wr = next;
}

/* ------------------------------------------------------------------ */
/* Init                                                                */
/* ------------------------------------------------------------------ */
void StmMonitor_init(void)
{
    IfxStm_initCompareConfig(&s_stmCfg);
    s_stmCfg.comparator          = IfxStm_Comparator_0;
    s_stmCfg.comparatorInterrupt = IfxStm_ComparatorInterrupt_ir0;
    s_stmCfg.compareOffset       = IfxStm_ComparatorOffset_0;
    s_stmCfg.compareSize         = IfxStm_ComparatorSize_32Bits;
    s_stmCfg.ticks               = (uint32)IfxStm_getTicksFromMicroseconds((PWM_Period) * 1e6);
    s_stmCfg.triggerPriority     = STM_MONITOR_ISR_PRIO;
    s_stmCfg.typeOfService       = IfxSrc_Tos_cpu2;

    IfxStm_initCompare(MONITOR_STM, &s_stmCfg);

    s_reloadTicks = s_stmCfg.ticks;

}
