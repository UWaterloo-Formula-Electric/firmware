#include "tim.h"

HAL_StatusTypeDef hallEffect_init(void) {
    if (HAL_TIM_Base_Init(&htim5) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_TIM_Base_Start(&htim5) != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef encoders_init(void) {
    if (hallEffect_init() != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}