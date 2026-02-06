#include "halleffectsensor.h"
#include <stdint.h>
#include "bsp.h"
#include "stm32f4xx_hal.h"

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == REAR_HALL_EFFECT_ENCODER_Pin) {
        cur_tick = HAL_GetTick();
        pulse_count++;
    }
}