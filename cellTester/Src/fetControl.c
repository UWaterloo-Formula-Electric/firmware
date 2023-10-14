#include <math.h>

#include "debug.h"
#include "stm32f0xx_hal.h"
#include "fetControl.h"

static float PWM_Duty_Cycle = 0;

HAL_StatusTypeDef set_PWM_Duty_Cycle(TIM_HandleTypeDef* const pwmHandle, const float duty_cycle) {
    if (duty_cycle < 0 || duty_cycle > 100) {
        DEBUG_PRINT("Invalid duty cycle: %f\n", duty_cycle);
        return HAL_ERROR;
    }
    PWM_Duty_Cycle = duty_cycle;
    // Get the Auto Reload Register value and set the compare register to a PWM percentage
    uint16_t currentARR = __HAL_TIM_GET_AUTORELOAD(pwmHandle);
    uint32_t nextARR = PWM_Duty_Cycle * currentARR /100.;
    __HAL_TIM_SET_COMPARE(pwmHandle, TIM_CHANNEL_1, nextARR);
    return HAL_OK;
}

float get_PWM_Duty_Cycle() {
    return PWM_Duty_Cycle;
}
// Sets the output to 0% duty cycle
// No current draw
HAL_StatusTypeDef fetInit() {
    HAL_TIM_PWM_Start(&FET_TIM_HANDLE, TIM_CHANNEL_1);
    return set_PWM_Duty_Cycle(&FET_TIM_HANDLE, 0);
}
