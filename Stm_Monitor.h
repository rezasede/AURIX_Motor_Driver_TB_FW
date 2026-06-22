/**
 * \file Stm_Monitor.h
 * \brief STM periodic timer (CPU2) that sends monitoring JSON over UDP from ISR
 */
#ifndef STM_MONITOR_H_
#define STM_MONITOR_H_

#include "Ifx_Types.h"

IFX_EXTERN void stm2MonitorISR (void);

#define MONITOR_PERIOD_MS        (10u)     /* 100 Hz */
#define STM_MONITOR_ISR_PRIO     (52u)     /* unique SRPN on CPU2 */

/* Must be called from CPU2 context, after Eth init + enableInterrupts(). */
void StmMonitor_init (void);

#endif /* STM_MONITOR_H_ */
