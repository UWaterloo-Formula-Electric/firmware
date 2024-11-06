#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "bsp.h"
#include "cmsis_os.h"
#include "debug.h"
#include "detectWSB.h"
#include "main.h"
#include "multiSensorADC.h"
#include "task.h"
#include "tim.h"
#if BOARD_ID == ID_WSBRL
#include "wsbrl_can.h"
#elif BOARD_ID == ID_WSBRR
#include "wsbrr_can.h"
#endif

#define NUM_TEETH 16
#define HALL_EFFECT_TASK_PERIOD 1000
#define WHEEL_RADIUS 0.40005
#define PI 3.14156
#define TICKS_PER_SECOND 1000 //todo: change back to 100
#define PERIOD ((float)HALL_EFFECT_TASK_PERIOD/TICKS_PER_SECOND)

/*
 * increments pulses every time a magnetic field is detected
 * timer config: (uses TIM1 Channel 3)
 *  - Channel: input capture direct mode
 *  - auto-reload-preload: Enable
 *  - Polarity Selection: Falling Edge
 *  - NVIC capture compare interrupt: enabled
 *  - NVIC update interrupt & TIM10 global interrupt: enabled
 *  - GPIO Pull up
 *  - GPIO open drain
 */


float getRps(uint32_t count) {
    /*
     * teeth/s = count diff/time diff
     * rps = 1/num_teeth * teeth/s
     */
    return count * (1.0f/PERIOD) * (1.0f/NUM_TEETH);
}

int64_t getRpm(float rps) {
    /*
     * rpm = 60rps
     * 2024 car: 16 teeth
     */
    return (int64_t)(int32_t)(rps * 60.0f);
}

float getKph(float rps) {
    /*
     * rad/s = 2pi * rps
     * m/s = rad/s * RADIUS
     * kph = 3.6 * m/s
     * 2024 car: effective radius 0.40005m
     */
    return rps * (2.0f*PI) * WHEEL_RADIUS * 3.6f;
}

uint32_t pulse_count = 0;
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    DEBUG_PRINT("capture callback called\n");
    if (htim->Instance == TIM1) {
        pulse_count++;
    }
}

void HallEffectSensorTask(void const * argument) {
    deleteWSBTask(WSBRL | WSBRR);
    DEBUG_PRINT("Starting HallEffectSensorTask\n");

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        float rps = getRps(pulse_count);
        int64_t rpm = getRpm(rps);
        float kph = getKph(rps);

#if BOARD_ID == ID_WSBRL
        RLSpeedRPM = rpm;
        RLSpeedKPH = kph;
        sendCAN_WSBRL_Speed();
#elif BOARD_ID == ID_WSBRR
        RRSpeedRPM = rpm;
        RRSpeedKPH = kph;
        sendCAN_WSBRR_Speed();
#endif

        pulse_count = 0;

        DEBUG_PRINT("Level: %d\n", HAL_GPIO_ReadPin(REAR_HALL_EFFECT_ENCODER_GPIO_Port, REAR_HALL_EFFECT_ENCODER_Pin));
        DEBUG_PRINT("RPM: %ld, KPH: %f, pulse counts: %ld\n", (int32_t)rpm, kph, pulse_count);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(HALL_EFFECT_TASK_PERIOD));
    }
}