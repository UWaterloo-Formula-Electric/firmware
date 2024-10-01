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
#if BOARD_ID == ID_WSBFL
#include "wsbfl_can.h"
#elif BOARD_ID == ID_WSBRR
#include "wsbrr_can.h"
#endif

#define NUM_TEETH 17
#define HALL_EFFECT_TASK_PERIOD 100

/*
 * IOC FILE CHANGES:
 * TIM4
 *  - Clock Source: internal clock
 *  - Combined Channels: Hall Sensor Mode
 *  - auto-reload preload: enable
 * SENSOR_IC_IN: PD12
 */

float getWheelSpeed() {
    /* hw timer set in hall effect mode, increments each time a gear passes by
     * reads count diff & time diff
     * teeth/s = count diff/time diff
     * rps = 1/num_teeth * teeth/s
     * rpm = 60rps
     * 2024 car: 17 teeth measured
     */
    return (1.0*TIM4->CNT) / (1.0*__HAL_TIM_GET_COUNTER(&htim4)) / (1.0*NUM_TEETH) * 60.0;
}

void StartHallEffectSensorTask(void const * argument) {
    DEBUG_PRINT("Starting StartHallEffectSensorTask\n");
    WSBType_t wsbType = detectWSB();
    if (wsbType != WSBRL || wsbType != WSBRR) {
        DEBUG_PRINT("Invalid wsb: not WSBFL or WSBRR, deleting StartHallEffectSensorTask\n");
        vTaskDelete(NULL);
        return;
    }

    HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
    uint32_t startTicks = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        float wheelRpm = getWheelSpeed();
        __HAL_TIM_SET_COUNTER(&htim4, 0);
        TIM4->CNT = 0;

#if BOARD_ID == ID_WSBRL

#elif BOARD_ID == ID_WSBRR

#endif

        DEBUG_PRINT("Wheel speed: %.2f\r\n", wheelRpm);
        
        vTaskDelayUntil(&xLastWakeTime, HALL_EFFECT_TASK_PERIOD);
        //todo: send can message
    }
}