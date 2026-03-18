/**
 * @file    Adc_MotorSense_Isr.c
 * @brief   ISR registration for 7-channel motor sensing ADC
 *
 * Call sequence:
 *   1. Pwm_init()                 -- EGTM TOM must exist before EGTM trigger wiring
 *   2. Adc_MotorSense_init()      -- TMADC + DMA + EGTM routing
 *   3. IfxCpu_enableInterrupts()
 *   4. Adc_MotorSense_start()     -- arm HW trigger, begin calibration
 *
 * After step 4, every PWM valley fires:
 *   t+0.0 µs  → current group converts  → ISR_CURR at IFX_INTPRIO_ADC_CURR (20)
 *   t+2.0 µs  → voltage group converts  → ISR_VOLT at IFX_INTPRIO_ADC_VOLT (18)
 */

#include "Adc_MotorSense.h"
#include "IfxCpu_Irq.h"
#include "Dma/Std/IfxDma.h"
#include "IfxPort.h"
#include "pin.h"

/* Current group result ISR — higher priority, TIME CRITICAL */
IFX_INTERRUPT(Adc_MotorSense_currIsr_handler, 0, IFX_INTPRIO_ADC_CURR)
{
    IfxPort_togglePin(IfxPort_P33_9.port, IfxPort_P33_9.pinIndex); //Debug GPIO for Scope


    /* Read VALID bit — if 0, conversion wasn't done when DMA fired */
    IFX_ASSERT(IFX_VERBOSE_LEVEL_ERROR,
        MODULE_ADC.TMADC[0].CH[ADC_CH_IC].RES.B.VF == 1);


    Adc_MotorSense_currIsr();
}

/* Voltage group result ISR — lower priority */
IFX_INTERRUPT(Adc_MotorSense_voltIsr_handler, 0, IFX_INTPRIO_ADC_VOLT)
{
    IfxPort_togglePin(IfxPort_P33_10.port, IfxPort_P33_10.pinIndex); //Debug GPIO for Scope


    Adc_MotorSense_voltIsr();
}
