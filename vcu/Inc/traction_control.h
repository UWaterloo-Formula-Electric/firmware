#ifndef TRACTION_CONTROL_H
#define TRACTION_CONTROL_H

#include "stm32f7xx_hal.h"

extern float tc_error_floor_rad_s;
extern float tc_torque_max_floor;
extern float tc_min_percent_error;

void toggle_TC(void);
void disable_TC(void);

HAL_StatusTypeDef tune_tc_PID(char controller, float val);
const float* get_tc_PID_gains(void);

extern float tc_kP;
extern float tc_kI;
extern float tc_kD;
extern float desired_slip;
extern float tc_pid_gains[3];
#endif
