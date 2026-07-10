/**
 * \file Control_Loops.c
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

#include "Control_Loops.h"

bool angleValid;
float speed;
uint16 CTRL_S = initial;
uint32_t theta;
FocAbc duty;

DapTune_Params DAP_IF;

void mainSWLoop (void)
{
    /* 2. overcurrent guard in software (hardware trip is the real one)      */
    g_currents.ic = -(g_currents.ia + g_currents.ib);
    if (g_currents.ia > FOC_I_MAX_A || g_currents.ia < -FOC_I_MAX_A || g_currents.ib > FOC_I_MAX_A
            || g_currents.ib < -FOC_I_MAX_A || g_currents.ic > FOC_I_MAX_A || g_currents.ic < -FOC_I_MAX_A)
    {
        Foc_disable();
        CTRL_S = protection;
        return;
    }

    /*------------------------------------------------------------------------*/
    /* The Main State Machine                                                 */
    /*------------------------------------------------------------------------*/
    switch (CTRL_S)
    {
        case initial :
            if (IfxPort_getPinState(BUTTONOB.port, BUTTONOB.pinIndex) == 0) //Switch on the board
            {
                CTRL_S = waitZ;
            }
            break;

        case protection :
            Foc_disable();
            break;

        case error :
            Foc_disable();
            break;

        case waitZ :
            if (EncoderCalib_service())
            {
                CTRL_S = running;
                Foc_setCurrentSetpoints(0, 0);    //set-points for FOC
            }
            break;

        case running :

            if (DapTune_read(&DAP_IF)) /* if something received by GUI*/
            {
                Foc_setCurrentSetpoints(DAP_IF.idRef_A, DAP_IF.iqRef_A); //set-points for FOC
                FocPi_init(&g_foc.piD, DAP_IF.kpD, DAP_IF.kiD);
                FocPi_init(&g_foc.piQ, DAP_IF.kpQ, DAP_IF.kiQ);
            }

            /*  encoder: position -> electrical angle + speed                      */
            EncoderCalib_service();
            theta = EncoderCalib_getElecAngle(&angleValid);
            speed = EncoderCalib_getElecSpeed(PWM_Period) * (float) MotorPolePairs;

            /*  the whole FOC pipeline                                             */
            Foc_isrStep(g_currents.ia, g_currents.ib, g_voltages.vdc, theta, speed, &duty);

            /* into the PWM compare SHADOW registers; hardware transfers them at  */
            /*    the next counter valley (t = 10 us in the time-line above)          */
            updateEgtmAtom3phInvDuty(duty.a * 100.0, duty.b * 100.0, duty.c * 100.0);

            g_dapTune.monitor.idMeas_A = g_foc.iDq.d;
            g_dapTune.monitor.iqMeas_A = g_foc.iDq.q;

            break;

            break;
        default :
            break;
    }

}

///* Pre-calculated Sweep Parameters to save CPU cycles in the ISR */
//// Constant used to convert frequency to radians per tick
//#define ANGLE_STEP_COEFF    (TWO_PI / PWM_FREQ_HZ)
///* Math Constants */
//#define TWO_PI              6.28318530718f
//#define PHASE_SHIFT_120     2.09439510239f
//
///* Configuration Parameters */
//#define PWM_FREQ_HZ         20000.0f
//#define START_FREQ_HZ       0.3f           // Starting frequency
//#define END_FREQ_HZ         10.0f          // Target highest frequency
//#define SWEEP_TIME_SEC      100.0f           // Time to reach target in seconds
//#define MAX_DUTY_TICKS      100           // Replace with your timer's 100% period

//// Static variables retain their state across ISR calls
//static float phase_angle = 0.0f;
//static float current_freq = START_FREQ_HZ;
//
//// 1. Linear Frequency Sweep (Ramp up)
//if (current_freq < END_FREQ_HZ)
//{
//    current_freq += 0;
//
//    // Clamp to exact target to avoid floating point overshoot
//    if (current_freq > END_FREQ_HZ)
//    {
//        current_freq = END_FREQ_HZ;
//    }
//}
//
//// 2. Calculate the dynamic angle step for this specific tick
//float angle_step = current_freq * ANGLE_STEP_COEFF;
//
//// 3. Advance the phase angle
//phase_angle += angle_step;
//if (phase_angle >= TWO_PI)
//{
//    phase_angle -= TWO_PI;
//}
//
//// 4. Calculate the 3-phase sine values (-1.0 to 1.0)
//float sine_R = sinf(phase_angle);
//float sine_S = sinf(phase_angle - PHASE_SHIFT_120);
//float sine_W = sinf(phase_angle + PHASE_SHIFT_120);
//
//// 5. Scale to Duty Cycle (0% to 95% limit preserved for bootstrap charging)
//uint32 duty_R = (uint32) ((sine_R + 1.0f) * 0.475f * MAX_DUTY_TICKS);
//uint32 duty_S = (uint32) ((sine_S + 1.0f) * 0.475f * MAX_DUTY_TICKS);
//uint32 duty_W = (uint32) ((sine_W + 1.0f) * 0.475f * MAX_DUTY_TICKS);
//
//// 6. Send to the PWM update function
//updateEgtmAtom3phInvDuty(duty_R, duty_S, duty_W);
//
//EncoderCalib_service();
//theta = EncoderCalib_getElecAngle(&angleValid);
//speed = EncoderCalib_getElecSpeed(PWM_Period);
