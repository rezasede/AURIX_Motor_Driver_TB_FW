#ifndef CONTROL_LOOPS_H_
#define CONTROL_LOOPS_H_

#include "Bsp.h"

#include <stdint.h>
#include <stdbool.h>
#include "math.h"

#include "Configurations.h"

#include "encoder_calib.h"
#include "EGTM_ATOM_3_Phase_Inverter_PWM.h"
#include "encoder_egtm.h"
#include "FOC.h"
#include "Adc_MotorSense.h"
#include "GPIO_LED_Button.h"
#include "dap_tune.h"

extern uint32_t theta;
extern float speed;
extern uint16 CTRL_S;
extern FocAbc duty;

extern volatile Adc_PhaseCurrents g_currents;
extern volatile Adc_PhaseVoltages g_voltages;

extern DapTune_Params DAP_IF;

typedef enum
{
    initial, waitZ, preparation, running, protection, error
} FOC_Status;

void mainSWLoop (void);

#endif
