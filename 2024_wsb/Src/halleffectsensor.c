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
#define HALL_EFFECT_TASK_PERIOD 100
#define WHEEL_RADIUS 0.40005
#define PI 3.14156
#define TICKS_PER_SECOND 1000

/*
 *
 * WSBRL CAN message: WSBRL_Speed
 *  signals:
 *      - int RLSpeedKPH
 *      - float RLSpeedRPM
 * WSBRR CAN message: WSBRR_Speed
 *  signals:
 *      - int RRSpeedKPH
 *      - float RRSpeedRPM
 */


//todo: cannot use float division
float getRpm(uint32_t ticks, uint32_t count) {
    /*
     * teeth/s = count diff/time diff
     * rps = 1/num_teeth * teeth/s
     * rpm = 60rps
     * 2024 car: 16 teeth
     */
    return (1.0*count) / (1.0*ticks/TICKS_PER_SECOND) / (1.0*NUM_TEETH) * 60.0;
}

uint16_t getKph(uint32_t ticks, uint32_t count) { //todo: confirm whether this needs rounding or not
    /*
     * teeth/s = count diff/time diff
     * rps = 1/num_teeth * teeth/s
     * rad/s = 2pi * rps
     * m/s = rad/s * RADIUS
     * kph = 3.6 * m/s
     * 2024 car: effective radius 0.40005m
     */
    return (uint16_t)((1.0*count) / (1.0*ticks/TICKS_PER_SECOND) / (1.0*NUM_TEETH) * (2*PI) * WHEEL_RADIUS * 3.6);
}

uint32_t pulse_count = 0;
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM1) {
        pulse_count++;
    }
}

uint32_t getTickDiff() {
    static uint32_t prev_tick = 0;

    uint32_t cur_tick = HAL_TIM_ReadCapturedValue(&htim1, TIM_CHANNEL_3);

    uint32_t time_diff = 0;
    if (cur_tick >= prev_tick) {
        time_diff = cur_tick-prev_tick;
    } else {
        time_diff = (0xFFFF-prev_tick) + cur_tick + 1;
    }

    prev_tick = cur_tick;

    return time_diff;
}

void HallEffectSensorTask(void const * argument) {
    DEBUG_PRINT("Starting StartHallEffectSensorTask\n");
    WSBType_t wsbType = detectWSB();
    if (wsbType != WSBRL && wsbType != WSBRR) {
        DEBUG_PRINT("Invalid wsb: not WSBRL or WSBRR, deleting HallEffectSensorTask\n");
        vTaskDelete(NULL);
        return;
    }

    HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_3); //todo: move this (await Aryan's input)

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        uint32_t ticks = getTickDiff();

        if (ticks == 0) {
            DEBUG_PRINT("ERROR: ticks = 0\r\n");
            vTaskDelayUntil(&xLastWakeTime, HALL_EFFECT_TASK_PERIOD);
            continue;
        }

        float rpm = getRpm(ticks, pulse_count);
        uint16_t kph = getKph(ticks, pulse_count);

        pulse_count = 0;

#if BOARD_ID == ID_WSBRL
        RLSpeedRPM = rpm;
        RLSpeedKPH = kph;
        sendCAN_WSBRL_Speed();
#elif BOARD_ID == ID_WSBRR
        RRSpeedRPM = rpm;
        RRSpeedKPH = kph;
        sendCAN_WSBRR_Speed();
#endif
        DEBUG_PRINT("RPM: %f, KPH: %u\n", rpm, kph);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(HALL_EFFECT_TASK_PERIOD));
    }
}