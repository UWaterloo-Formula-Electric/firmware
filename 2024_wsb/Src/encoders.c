#include "stm32f4xx_hal.h"
#include "tim.h"

HAL_StatusTypeDef encoders_init(void) {
    // add your timer enable code here
    if (HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_3) != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}