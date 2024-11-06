#include "stm32f4xx_hal.h"
#include "tim.h"

HAL_StatusTypeDef encoders_init(void) {
    if (HAL_TIM_Base_Start_IT(&htim1) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_3) != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}