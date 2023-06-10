#ifndef TRACTION_CONTROL_H
#define TRACTION_CONTROL_H

extern float tc_kP;
extern float tc_error_floor_rad_s;
extern float tc_torque_max_floor;
extern float tc_min_percent_error;

void toggle_TC(void);
void disable_TC(void);
extern float tc_kP;
extern float tc_kI;
extern float tc_kD;
extern float desired_slip;
#endif
