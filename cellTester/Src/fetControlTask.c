#include "fetControlTask.h"
#include "stm32f0xx_hal.h"
// #include "task.h"
#include "debug.h"
#include <math.h>

static float PWM_Duty_Cycle = 0;

HAL_StatusTypeDef set_PWM_Duty_Cycle(TIM_HandleTypeDef* const pwmHandle, const float duty_cycle) {
    if (duty_cycle < 0 || duty_cycle > 100) {
        DEBUG_PRINT("Invalid duty cycle: %f", duty_cycle);
        return HAL_ERROR;
    }
    // The duty cycle value is a percentage of the reload register value (ARR). Rounding is used.
    uint32_t newRegVal = (uint32_t)roundf((double)(pwmHandle->Instance->ARR) * (duty_cycle * 0.01F));

    // In case of the DC being calculated as higher than the reload register, cap it to the reload register
    if(newRegVal > pwmHandle->Instance->ARR){
        newRegVal = pwmHandle->Instance->ARR;
    }

    PWM_Duty_Cycle = duty_cycle;
    // Assign the new DC count to the capture compare register.
    pwmHandle->Instance->CCR1 = (uint32_t)(roundf(newRegVal));
    return HAL_OK;
}

// Sets the output to 0% duty cycle
// No current draw
HAL_StatusTypeDef fetInit(){
    HAL_TIM_PWM_Start(&FET_TIM_HANDLE, TIM_CHANNEL_1);
    return set_PWM_Duty_Cycle(&FET_TIM_HANDLE, 0);
}

void fetControlTask(void const *argument) {
    
    while (1) {
        DEBUG_PRINT("PWM Duty Cycle: %f", PWM_Duty_Cycle);
    }
}