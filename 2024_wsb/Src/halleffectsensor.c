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
#define PERIOD = HALL_EFFECT_TASK_PERIOD/TICKS_PER_SECOND

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


float getRps(uint32_t count) {
    /*
     * teeth/s = count diff/time diff
     * rps = 1/num_teeth * teeth/s
     */
    return count * (1.0f/PERIOD) * (1.0f/NUM_TEETH);
}

float getRpm(uint32_t rps) {
    /*
     * rpm = 60rps
     * 2024 car: 16 teeth
     */
    return rps * 60.0;
}

uint16_t getKph(uint32_t rps) { //todo: confirm whether this needs rounding or not
    /*
     * rad/s = 2pi * rps
     * m/s = rad/s * RADIUS
     * kph = 3.6 * m/s
     * 2024 car: effective radius 0.40005m
     */
    return (uint16_t)(rps * (2*PI) * WHEEL_RADIUS * 3.6);
}

uint32_t pulse_count = 0;
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM1) {
        pulse_count++;
    }
}

void HallEffectSensorTask(void const * argument) {
    DEBUG_PRINT("Starting StartHallEffectSensorTask\n");
    WSBType_t wsbType = detectWSB();
    if (wsbType != WSBRL && wsbType != WSBRR) {
        DEBUG_PRINT("Invalid wsb: not WSBRL or WSBRR, deleting HallEffectSensorTask\n");
        vTaskDelete(NULL);
        return;
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {

        float rps = getRps(pulse_count);
        float rpm = getRpm(rps);
        uint16_t kph = getKph(rps);

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