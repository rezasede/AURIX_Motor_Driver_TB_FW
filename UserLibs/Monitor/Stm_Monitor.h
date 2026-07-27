/**
 * \file Stm_Monitor.h
 * \brief STM periodic timer on CPU2, period = PWM period (50 us):
 *          every tick : sample motor signals -> ring buffer
 *                       drain <=2 samples -> JSON UDP (lossless, <=1ms latency)
 *          every 1 ms : Ethernet service
 *        Loss detection: "n" counter in JSON + g_monRing.overrun on MCU.
 */
#ifndef STM_MONITOR_H_
#define STM_MONITOR_H_

#include "Ifx_Types.h"

/* ------------------------------------------------------------------ */
/* Configuration                                                       */
/* ------------------------------------------------------------------ */
#define STM_MONITOR_ISR_PRIO        (52u)   /* unique SRPN on CPU2 */

#define MON_RING_SIZE               (512u)  /* power of two */
#define MON_RING_MASK               (MON_RING_SIZE - 1u)

/* Max JSON datagrams sent per tick. Production = 1 sample/tick,
 * so 2 gives 2x drain headroom. Raise only if ISR timing allows. */
#define MON_MAX_SENDS_PER_TICK      (2u)

/* Set to 1 to use the binary bulk UDP path instead of JSON
 * (full rate, ~4x less bandwidth, needs PC bridge). */
#define MON_USE_BINARY_BULK         (1)

/* ------------------------------------------------------------------ */
/* Types                                                               */
/* ------------------------------------------------------------------ */
/* THE ONE LIST. Add/remove signals ONLY here. Order matters. */
#define MON_SIGNAL_LIST(X)   \
    X(ia,    g_currents.ia)  \
    X(ib,    g_currents.ib)  \
    X(ic,    g_currents.ic)  \
    X(va,    g_voltages.va)  \
    X(vb,    g_voltages.vb)  \
    X(vc,    g_voltages.vc)  \
    X(vdc,   g_voltages.vdc) \
    X(id,    g_foc.iDq.d)    \
    X(iq,    g_foc.iDq.q)    \
    X(theta, (float32)theta) \
    X(speed, speed) \
    X(AUX1, (float32)AUX1)

/* struct generated from the list */
typedef struct
{
    uint32  t_us;
#define AS_MEMBER(name, src)  float32 name;
    MON_SIGNAL_LIST(AS_MEMBER)
#undef AS_MEMBER
    uint32  status;
} MonSample;

/* count generated from the list */
#define AS_COUNT(name, src)  + 1
#define MON_NUM_FLOATS  (0 MON_SIGNAL_LIST(AS_COUNT))

_Static_assert(sizeof(MonSample) == (MON_NUM_FLOATS + 2u) * 4u,"padding crept in - only float32/uint32 allowed");

#define SAMPLES_PER_PKT  ((1472u - 12u) / sizeof(MonSample))   /* auto MTU fit */

typedef struct
{
    volatile uint32 wr;              /* written only by producer       */
    volatile uint32 rd;              /* written only by consumer       */
    volatile uint32 overrun;         /* dropped samples; must stay 0   */
    MonSample buf[MON_RING_SIZE];
} MonRing;

extern MonRing g_monRing;
extern volatile uint32 g_monMissedTicks;

/* ------------------------------------------------------------------ */
/* API                                                                 */
/* ------------------------------------------------------------------ */
IFX_EXTERN void stm2MonitorISR(void);

/** Must be called from CPU2 context, after Eth init + enableInterrupts(). */
void StmMonitor_init(void);

#endif /* STM_MONITOR_H_ */
