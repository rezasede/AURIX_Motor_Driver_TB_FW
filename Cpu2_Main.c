/**
 * \file Cpu2_Main.c
 * \brief Core 2 - Ethernet + OneEye BB protocol over UDP
 */
#include "Ifx_Types.h"
#include "Ifx_Cfg.h"
#include "IfxCpu.h"
#include "IfxWtu.h"

#include "Adc_MotorSense.h"
#include "ASCLIN_UART.h"
#include "UserFunctions.h"
#include <math.h>
#include "Eth_Core.h"

#include "Monitor_Json.h"
#include "Stm_Monitor.h"
#include "GPIO_LED_Button.h"

extern volatile boolean g_sendJsonTrigger; /* Aus dem Monitor-File */
static char udpBuffer[256];
static MonitorData myData;

extern IfxCpu_syncEvent g_cpuSyncEvent;
extern volatile boolean g_cpu0InitDone;

void core2_main (void)
{
    IfxWtu_disableCpuWatchdog(IfxWtu_getCpuWatchdogPassword());
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    while (g_cpu0InitDone == FALSE)
    {
    }

    init_ASCLIN_UART();         // Baudrate: 576000 bps

    /*=== Initialize Ethernet ===*/
    Eth_Core2_Config ethCfg;
    Eth_Core2_getDefaultConfig(&ethCfg);    // IP: 192.168.178.100 // Port:9870

    if (Eth_Core2_init(&ethCfg) == FALSE)
    {
        sendUART("ETH FAIL\r\n", 10);
    }

    uint32 fcx = __mfcr(CPU_FCX); /* free CSA list head  (0xFE38) */
    uint32 lcx = __mfcr(CPU_LCX); /* CSA list limit      (0xFE3C) */
    uint32 biv = __mfcr(CPU_BIV); /* interrupt vector base (0xFE20) */


    initLED();

    IfxCpu_enableInterrupts();

    StmMonitor_init();

    while (1)
    {

    }
}
