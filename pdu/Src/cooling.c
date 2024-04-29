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

int get_duration_from_temp(float temp, float max_temp) {
    float interpolated_percent = (((temp) - COOLING_AMBIENT_TEMP_C) / ((max_temp) - COOLING_AMBIENT_TEMP_C));
    int cool_duration_ms = interpolated_percent * COOLING_TASK_PERIOD_MS;
    return cool_duration_ms;
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
        if (fsmGetState(&mainFsmHandle) == STATE_Motors_On) {
            float avg_inv_module_temp = (INV_Module_A_Temp + INV_Module_B_Temp + INV_Module_C_Temp) / 3;
            int motor_duration_ms = get_duration_from_temp(INV_Motor_Temp, COOLING_MOTOR_MAX_TEMP_C);
            int inv_duration_ms = get_duration_from_temp(avg_inv_module_temp, COOLING_INV_MAX_TEMP_C);

            // take maximum duration
            int cooling_duration_ms = MAX(motor_duration_ms, inv_duration_ms);
            if (cooling_duration_ms > COOLING_TASK_PERIOD_MS) cooling_duration_ms = COOLING_TASK_PERIOD_MS;

            if (cooling_duration_ms > 0) {
                DEBUG_PRINT("Toggle cooling for %d ms\n", cooling_duration_ms);
                coolingOn();
                vTaskDelay(pdMS_TO_TICKS(cooling_duration_ms));
                coolingOff();
            }
        }
        watchdogTaskCheckIn(COOLING_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(COOLING_TASK_PERIOD_MS));
    }
}
