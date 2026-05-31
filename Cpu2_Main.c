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
#include "Eth_OneEye.h"

#include "GPIO_LED_Button.h"

/*=== TEST MODE SWITCH ===
 * TEST_RAW_UDP      -> raw struct to Hercules on port 50001
 * TEST_BB_DIAG      -> BB protocol with UART debug prints (find what's broken)
 * Neither defined   -> production BB protocol (no debug output)
 */
//#define TEST_RAW_UDP
#define TEST_BB_DIAG

double x_float = 0.0f;
double y_float = 0.0f;
uint64 t = 0;

extern IfxCpu_syncEvent g_cpuSyncEvent;
extern volatile boolean g_cpu0InitDone;

void core2_main (void)
{
    IfxWtu_disableCpuWatchdog(IfxWtu_getCpuWatchdogPassword());
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    while (g_cpu0InitDone == FALSE)
        ;

    init_ASCLIN_UART();

    /*=== Initialize Ethernet ===*/
    Eth_Core2_Config ethCfg;
    Eth_Core2_getDefaultConfig(&ethCfg);

    if (Eth_Core2_init(&ethCfg) == FALSE)
    {
        sendUART("ETH FAIL\r\n", 10);
        while (1)
            ;
    }

    sendUART("ETH OK\r\n", 8);

    /*=== Initialize OneEye ===*/
    OneEye_init();

    sendUART("OE OK\r\n", 7);

    IfxCpu_enableInterrupts();

    OneEye_MotorData motorData;

    while (1)
    {
        x_float = 10.0f * sinf(2.0f * 3.14159f * 0.001f * t + 1000);
        y_float = 10.0f * sinf(2.0f * 3.14159f * 0.001f * t);

        motorData.x_raw = (uint32) ((x_float + 10.0f) * 3276.7f);
        motorData.y_raw = (uint32) ((y_float + 10.0f) * 3276.7f);

        const Adc_PhaseCurrents *pCurr = Adc_MotorSense_getCurrents();
        motorData.z_raw = (uint32) pCurr->raw_diff[0];

        t++;

        Eth_Core2_service1ms();

//        /*--- Raw UDP test (Hercules port 50001) ---*/
//        if (Eth_Core2_isLinkUp())
//        {
//            Eth_Core2_sendUdp(&motorData, sizeof(motorData));
//        }

        /*--- Production mode ---*/
        if (Eth_Core2_isLinkUp())
        {
            OneEye_sendMotorData(&motorData);
        }

        OneEye_service();

        /* Print diagnostics every 500ms */
        if ((t % 500) == 1)
        {
            if (Eth_Core2_isLinkUp())
            {
                sendUART("L", 1);
            }

            /* A=addClient result (1=ok, 0=fail) */
            char a = '0' + (char) OneEye_getDiagAddClientOk();
            sendUART("A", 1);
            sendUART(&a, 1);
            sendUART(" ", 1);

            /* Q=queued ok, F=queue failed */
            uint32 qOk = OneEye_getDiagSendOk();
            uint32 qFail = OneEye_getDiagSendFail();
            char buf[24];
            buf[0] = 'Q';
            buf[1] = '0' + (char) ((qOk / 1000) % 10);
            buf[2] = '0' + (char) ((qOk / 100) % 10);
            buf[3] = '0' + (char) ((qOk / 10) % 10);
            buf[4] = '0' + (char) (qOk % 10);
            buf[5] = ' ';
            buf[6] = 'F';
            buf[7] = '0' + (char) ((qFail / 1000) % 10);
            buf[8] = '0' + (char) ((qFail / 100) % 10);
            buf[9] = '0' + (char) ((qFail / 10) % 10);
            buf[10] = '0' + (char) (qFail % 10);
            buf[11] = ' ';

            /* W=total bytes written to DPipe */
            uint32 w = OneEye_getDiagWriteCount();
            buf[12] = 'W';
            buf[13] = '0' + (char) ((w / 10000) % 10);
            buf[14] = '0' + (char) ((w / 1000) % 10);
            buf[15] = '0' + (char) ((w / 100) % 10);
            buf[16] = '0' + (char) ((w / 10) % 10);
            buf[17] = '0' + (char) (w % 10);
            buf[18] = '\r';
            buf[19] = '\n';
            sendUART(buf, 20);
        }

        Delay(1);
    }
}
