/**
 * \file CONFIGURATIONS.h
 */
#ifndef CONFIGURATIONS_H_
#define CONFIGURATIONS_H_

#define PWM_FREQUENCY   (40.0e3)
#define PWM_Period      (1.0 / PWM_FREQUENCY)
#define PWM_DeadTime_Rise     75e-9f
#define PWM_DeadTime_Fall     75e-9f

#define ENC_CAL    0
#define MotorPolePairs 7

#endif /* CONFIGURATIONS_H_ */
