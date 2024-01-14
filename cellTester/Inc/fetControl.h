#ifndef __FET_CONTROL_H__
#define __FET_CONTROL_H__

#include "stm32f0xx_hal.h"

HAL_StatusTypeDef fetInit();
HAL_StatusTypeDef set_PWM_Duty_Cycle(TIM_HandleTypeDef* const pwmHandle, const float duty_cycle);
float get_PWM_Duty_Cycle();
#endif // __FET_CONTROL_H__