#ifndef EGTM_ATOM_3_PHASE_INVERTER_PWM_H_
#define EGTM_ATOM_3_PHASE_INVERTER_PWM_H_


#include "Ifx_Types.h"


#define NUM_OF_CHANNELS         (3)
#define PWM_FREQUENCY           (20.0E3)
#define ISR_PRIORITY_ATOM       (20)

#define PHASE_U_HS              &IfxEgtm_ATOM0_0_TOUT64_P20_8_OUT
#define PHASE_U_LS              &IfxEgtm_ATOM0_0N_TOUT65_P20_9_OUT
#define PHASE_V_HS              &IfxEgtm_ATOM0_1_TOUT66_P20_10_OUT
#define PHASE_V_LS              &IfxEgtm_ATOM0_1N_TOUT67_P20_11_OUT
#define PHASE_W_HS              &IfxEgtm_ATOM0_2_TOUT68_P20_12_OUT
#define PHASE_W_LS              &IfxEgtm_ATOM0_2N_TOUT69_P20_13_OUT

#define PHASE_U_DUTY            (50.0f)
#define PHASE_V_DUTY            (50.0f)
#define PHASE_W_DUTY            (50.0f)

void initEgtmAtom3phInv(void);
void updateEgtmAtom3phInvDuty (uint32 Duty_R, uint32 Duty_S, uint32 Duty_W);
void startPWMinEGTM(void);

#endif
