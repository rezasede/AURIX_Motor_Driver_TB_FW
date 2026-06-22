/**
 * \file Stm_Monitor.c
 * \brief Single 1ms STM interrupt on CPU2: services Ethernet + sends JSON.
 *        All GETH access happens in this one context — no concurrency.
 */
#include "Stm_Monitor.h"
#include "IfxStm.h"
#include "IfxCpu.h"
#include "IfxSrc.h"
#include "Monitor_Json.h"
#include "Eth_Core.h"
#include "Adc_MotorSense.h"
#include <math.h>

#define MONITOR_STM        (&MODULE_CPU2)
#define TICK_PERIOD_MS     (1u)                 /* base tick */

static IfxStm_CompareConfig s_stmCfg;
static IfxStm_CompareConfig s_stmCfg;

/* File-scope in Stm_Monitor.c — NOT on the ISR stack */
static MonitorData s_monitorData;
static char s_udpBuffer[256];

IFX_INTERRUPT(stm2MonitorISR, 2, STM_MONITOR_ISR_PRIO)
{
    /* Re-arm the timer for the next 1 ms tick */
    IfxStm_clearCompareFlag(MONITOR_STM, s_stmCfg.comparator);
    IfxStm_increaseCompare(MONITOR_STM, s_stmCfg.comparator, s_stmCfg.ticks);

    /* (1) Service the Ethernet driver every 1 ms.
     *     Touches MODULE_GETH0 (link monitor / MDIO, RX). */
    Eth_Core2_service1ms();

    /* (2) Every 10 ticks (-> 100 Hz) build and send one UDP frame.
     *     Same context as service1ms above, so GETH is never accessed
     *     concurrently — no reentrancy hazard. */
    static uint32 tickCounter = 0;
    if (++tickCounter >= 10u)
    {
        tickCounter = 0;

        if (Eth_Core2_isLinkUp())
        {
            s_monitorData.t = IfxStm_getMicrosecondsFromTicks(IfxStm_get(MONITOR_STM)); /* or a real timestamp */
            s_monitorData.signals[0].name = "x";
            s_monitorData.signals[0].value = 1.23f;
            s_monitorData.signals[1].name = "y";
            s_monitorData.signals[1].value = -4.56f;
            s_monitorData.numSignals = 2;

            uint32 len = MonitorJson_build(&s_monitorData, s_udpBuffer, sizeof(s_udpBuffer));
            if (len > 0u)
            {
                Eth_Core2_sendUdp(s_udpBuffer, len);
            }
        }
    }
}

void StmMonitor_init (void)
{
    IfxStm_initCompareConfig(&s_stmCfg);

    s_stmCfg.comparator = IfxStm_Comparator_0;
    s_stmCfg.comparatorInterrupt = IfxStm_ComparatorInterrupt_ir0;
    s_stmCfg.compareOffset = IfxStm_ComparatorOffset_0;
    s_stmCfg.compareSize = IfxStm_ComparatorSize_32Bits;
    s_stmCfg.ticks = (uint32) IfxStm_getTicksFromMilliseconds(TICK_PERIOD_MS);
    s_stmCfg.triggerPriority = STM_MONITOR_ISR_PRIO;
    s_stmCfg.typeOfService = IfxSrc_Tos_cpu2;

    IfxStm_initCompare(MONITOR_STM, &s_stmCfg);
}
