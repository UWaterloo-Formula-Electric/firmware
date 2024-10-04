#include <stdio.h>
#include <string.h>

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

#define NUM_TEETH 17
#define HALL_EFFECT_TASK_PERIOD 100
#define WHEEL_RADIUS 0.40005
#define PI 3.14156

/*
 * - hw timer set in hall effect mode, increments each time a gear passes by
 * - reads count diff & time diff
 * IOC FILE CHANGES:
 * - TIM4
 *  - Clock Source: internal clock
 *  - Combined Channels: Hall Sensor Mode
 *  - auto-reload preload: enable
 * - SENSOR_IC_IN: PD12
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

float getRpm(uint32_t ticks, uint32_t count) {
    /*
     * teeth/s = count diff/time diff
     * rps = 1/num_teeth * teeth/s
     * rpm = 60rps
     * 2024 car: 17 teeth measured
     */
    return (1.0*count) / (1.0*ticks) / (1.0*NUM_TEETH) * 60.0;
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
    return (uint16_t)((1.0*count) / (1.0*ticks) / (1.0*NUM_TEETH) * (2*PI) * WHEEL_RADIUS * 3.6);
}

void StartHallEffectSensorTask(void const * argument) {
    DEBUG_PRINT("Starting StartHallEffectSensorTask\n");
    WSBType_t wsbType = detectWSB();
    if (wsbType != WSBRL || wsbType != WSBRR) {
        DEBUG_PRINT("Invalid wsb: not WSBRL or WSBRR, deleting StartHallEffectSensorTask\n");
        vTaskDelete(NULL);
        return;
    }

    HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL); //todo: move this
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        uint32_t count = TIM4->CNT;
        uint32_t ticks = __HAL_TIM_GET_COUNTER(&htim4);

        if (ticks == 0) {
            DEBUG_PRINT("ERROR: ticks = 0\r\n");
            vTaskDelayUntil(&xLastWakeTime, HALL_EFFECT_TASK_PERIOD);
            continue;
        }

        float rpm = getRpm(ticks, count);
        uint16_t kph = getKph(ticks, count);

        __HAL_TIM_SET_COUNTER(&htim4, 0);
        TIM4->CNT = 0;

#if BOARD_ID == ID_WSBRL
        RLSpeedRPM = rpm;
        RLSpeedKPH = kph;
        sendCAN_WSBRL_Speed();
#elif BOARD_ID == ID_WSBRR
        RRSpeedRPM = rpm;
        RRSpeedKPH = kph;
        sendCAN_WSBRR_Speed();
#endif
        printf("RPM: %f, KPH: %u\n", rpm, kph);
        vTaskDelayUntil(&xLastWakeTime, HALL_EFFECT_TASK_PERIOD);
    }
}