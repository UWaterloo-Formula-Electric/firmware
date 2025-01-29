#include "callbacks.h"

#include <stdint.h>

#include "bsp.h"
#include "detectWSB.h"
#include "encoders.h"
#include "stm32f4xx_hal.h"

volatile uint16_t rearHallTicksCounter = 0;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == REAR_HALL_EFFECT_ENCODER_Pin) {
        rearHallTicksCounter++;
    }
}

// called in main.c in the HAL_TIM_PeriodElapsedCallback
void encoderRPSCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == ENCODER_RPS_TIM_HANDLE.Instance) {
        static uint16_t last = 0;
        float diff = 0;
        uint16_t curr = 0;

        // An assumption is made that the wheels only spin in the forward direction and never backwards
        if (detectWSB() & (WSBFL | WSBFR)) {
            curr = __HAL_TIM_GET_COUNTER(&ENCODER_TIM_HANDLE);
        } else if (detectWSB() & (WSBRL | WSBRR)) {
            curr = rearHallTicksCounter;
        }

        // handle counter overflow
        if (curr < last) {
            diff = 0XFFFFul - (last - curr);
        } else {
            diff = curr - last;
        }

        if (detectWSB() & (WSBFL | WSBFR)) {
            frontRPS = diff * SAMPLES_PER_SEC / TICKS_PER_QUAD_ENC_REV;
        } else if (detectWSB() & (WSBRL | WSBRR)) {
            rearRPS = diff * SAMPLES_PER_SEC / NUM_REAR_SPOKE_TEETH;
        }

        last = curr;
    }
}