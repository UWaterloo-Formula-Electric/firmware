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
#define TICKS_PER_SECOND 1000

/*
 * IOC FILE CHANGES:
 * PE13:
 *  state: GPIO_EXTI13
 *  GPIO MODE: external interrupt mode w/ pull-up resistor
 *  user label: REAR_HALL_EFFECT_ENCODER
 *  EXTI[15:10] interrupts: enabled
 */


float getRps(uint32_t count, uint32_t tick_diff) {
    /*
     * teeth/s = count diff/time diff
     * rps = 1/num_teeth * teeth/s
     */
    return (1.0f*count) / (1.0f*tick_diff/TICKS_PER_SECOND) / NUM_TEETH;
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
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == REAR_HALL_EFFECT_ENCODER_Pin) {
        pulse_count++;
    }
}

void HallEffectSensorTask(void const * argument) {
    deleteWSBTask(WSBRL | WSBRR);
    DEBUG_PRINT("Starting HallEffectSensorTask\n");

    TickType_t xLastWakeTime = xTaskGetTickCount();

    uint32_t last_tick = HAL_GetTick();
    uint32_t cur_tick;
    uint32_t tick_diff;

    while (1) {
        cur_tick = HAL_GetTick();
        tick_diff = cur_tick - last_tick;

        float rps = getRps(pulse_count, tick_diff);
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

        DEBUG_PRINT("Level: %d\n", HAL_GPIO_ReadPin(REAR_HALL_EFFECT_ENCODER_GPIO_Port, REAR_HALL_EFFECT_ENCODER_Pin));
        DEBUG_PRINT("RPM: %ld, KPH: %f, pulse counts: %ld\n", (int32_t)rpm, kph, pulse_count);
        pulse_count = 0;
        last_tick = HAL_GetTick();

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(HALL_EFFECT_TASK_PERIOD));
    }
}