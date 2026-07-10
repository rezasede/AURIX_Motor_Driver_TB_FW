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
#include "GPIO_LED_Button.h"
#include "Control_Loops.h"
#include "encoder_calib.h"

#define MONITOR_STM        (&MODULE_CPU2)
#define TICK_PERIOD_MS     (1u)                 /* base tick */

static IfxStm_CompareConfig s_stmCfg;
static uint32 s_reloadTicks;

/* File-scope in Stm_Monitor.c */
static MonitorData s_monitorData;
static char s_udpBuffer[512u];

extern volatile Adc_PhaseCurrents g_currents;
extern volatile Adc_PhaseVoltages g_voltages;

IFX_INTERRUPT(stm2MonitorISR, 2, STM_MONITOR_ISR_PRIO)
{
    /* Re-arm the timer for the next 1 ms tick */
    IfxStm_clearCompareFlag(MONITOR_STM, s_stmCfg.comparator);
    IfxStm_increaseCompare(MONITOR_STM, s_stmCfg.comparator, s_reloadTicks);

    toggleLed(LED_1s);

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
            s_monitorData.t = IfxStm_getMicrosecondsFromTicks(IfxStm_get(MONITOR_STM)) / 1000; /* or a real timestamp */

            s_monitorData.signals[0].name = "ia";
            s_monitorData.signals[0].value = g_currents.ia;
            s_monitorData.signals[1].name = "ib";
            s_monitorData.signals[1].value = g_currents.ib;
            s_monitorData.signals[2].name = "ic";
            s_monitorData.signals[2].value = g_currents.ic;

            s_monitorData.signals[3].name = "va";
            s_monitorData.signals[3].value = g_voltages.va;
            s_monitorData.signals[4].name = "vb";
            s_monitorData.signals[4].value = g_voltages.vb;
            s_monitorData.signals[5].name = "vc";
            s_monitorData.signals[5].value = g_voltages.vc;
            s_monitorData.signals[6].name = "vdc";
            s_monitorData.signals[6].value = g_voltages.vdc;

            s_monitorData.signals[7].name = "Id";
            s_monitorData.signals[7].value = g_foc.iDq.d;
            s_monitorData.signals[8].name = "Iq";
            s_monitorData.signals[8].value = g_foc.iDq.q;

            s_monitorData.signals[9].name = "Status";
            s_monitorData.signals[9].value = (uint16) CTRL_S;

            s_monitorData.signals[10].name = "theta";
            s_monitorData.signals[10].value = (uint32_t) theta;

            s_monitorData.signals[11].name = "speed";
            s_monitorData.signals[11].value = speed;

            s_monitorData.numSignals = 12;

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

    s_reloadTicks = s_stmCfg.ticks; /* snapshot, used by the ISR instead of s_stmCfg.ticks */
}
