/**
 * @file    Adc_MotorSense_Isr.c
 * @brief   ISR registration for unified 10-channel motor sensing ADC
 *
 * Call sequence (must be respected):
 *   1. initEgtmAtom3phInv()        ← EGTM must exist before ADC trigger wiring
 *   2. Adc_MotorSense_init()       ← TMADC + DMA + EGTM trigger routing
 *   3. IfxCpu_enableInterrupts()
 *   4. Adc_MotorSense_start()      ← runs calibration then arms HW trigger
 *   5. startPWMinEGTM()            ← begins PWM output and ADC trigger generation
 *
 * Per-PWM-valley event chain:
 *   ATOM Ch3 rising edge (PWM valley)
 *     → All 10 S&H freeze simultaneously (same HWTRxMS, HWTRDLYEN = 0)
 *     → Core-0 and Core-1 convert 5 channels each in parallel
 *     → Each conversion result fires SR_0 → DMA transfers one 32-bit word
 *     → After 10 DMA transfers:
 *         → DMA raises CPU ISR at IFX_INTPRIO_ADC_ALL (priority 50)
 *         → Adc_MotorSense_allIsr() computes diff currents + voltages
 *         → DMA re-armed for next cycle
 *
 * Debug GPIO usage:
 *   P33_9 toggles on every ISR entry — measure on oscilloscope to verify:
 *     - Period matches PWM period (5 µs at 200 kHz)
 *     - Jitter is sub-100 ns (confirms HW trigger, not SW)
 *     - Pulse width indicates ISR execution time
 */

#include "Adc_MotorSense.h"
#include "IfxCpu_Irq.h"
#include "IfxPort.h"
#include "pin.h"

/*══════════════════════════════════════════════════════════════════════════
 * UNIFIED ADC ISR
 *   Fires on CPU0 after all 10 DMA transfers from TMADC result registers
 *   to g_dmaAll are complete.
 *══════════════════════════════════════════════════════════════════════════*/
IFX_INTERRUPT(Adc_MotorSense_allIsr_handler, 0, IFX_INTPRIO_ADC_ALL)
{
    /*
     * Debug toggle — measures ISR period and jitter on a scope.
     * Remove in production to save ~10 ns per cycle.
     */
    IfxPort_togglePin(IfxPort_P33_9.port, IfxPort_P33_9.pinIndex);

    Adc_MotorSense_allIsr();
}
