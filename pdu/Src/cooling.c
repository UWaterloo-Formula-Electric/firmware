#include "cooling.h"

#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "FreeRTOS_CLI.h"
#include "debug.h"
#include "pdu_can.h"
#include "watchdog.h"
#include "canReceive.h"
#include "pdu_dtc.h"
#include "LTC4110.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SCALE_TEMP_PERCENT(temp, max_temp) (((temp) - COOLING_AMBIENT_TEMP_C) / ((max_temp) - COOLING_AMBIENT_TEMP_C))

void coolingOff(void) {
    DEBUG_PRINT("Turning cooling off\n");
    PUMP_LEFT_DISABLE;
    PUMP_RIGHT_DISABLE;
    FAN_LEFT_DISABLE;
    FAN_RIGHT_DISABLE;
}

void coolingOn(void) {
    DEBUG_PRINT("Turning cooling on\n");
    PUMP_LEFT_ENABLE;
    PUMP_RIGHT_ENABLE;
    FAN_LEFT_ENABLE;
    FAN_RIGHT_ENABLE;
}

void coolingTask(void *pvParameters) {
    if (registerTaskToWatch(COOLING_TASK_ID, 2*COOLING_TASK_PERIOD_MS, false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register cooling task with watchdog!\n");
        Error_Handler();
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        // Only cool if motor controller on and EM enabled
        // Should we check if MC is on?
        if (fsmGetState(&mainFsmHandle) == STATE_Motors_On) {
            coolingOn();
        }
        watchdogTaskCheckIn(COOLING_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(COOLING_TASK_PERIOD_MS));
    }
}
