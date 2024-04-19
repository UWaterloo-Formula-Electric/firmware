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

    uint32_t cooling_state = COOL_STATE_WAIT;

    while(1)
    {
        // Only cool if motor controller on and EM enabled
        // Should we check if MC is on?
        if (fsmGetState(&mainFsmHandle) == STATE_Motors_On && DC_DC_state) {
            cooling_state = COOL_STATE_ON;
            DEBUG_PRINT("Cooling enabled\n");
        } else {
            cooling_state = COOL_STATE_OFF;
            DEBUG_PRINT("Cooling disabled\n");
        }

        if (cooling_state == COOL_STATE_ON)
        {
            // Check if we are receiving temp readings
            if (xTaskGetTickCount() - MC_Last_Temperature_Msg_ticks > pdMS_TO_TICKS(MC_MESSAGE_LOST_THRESHOLD_MS)) {
                sendDTC_WARNING_MC_Temp_Reading_Lost();
            }

            // motor temp
            float scaled_motor_temp_percent = SCALE_TEMP_PERCENT(INV_Motor_Temp, COOLING_MOTOR_MAX_TEMP_C);
            int motor_duration_ms = scaled_motor_temp_percent * COOLING_TASK_PERIOD_MS;
            // inverter temp
            float average_inv_module_temp = (INV_Module_A_Temp + INV_Module_B_Temp + INV_Module_C_Temp) / 3;
            float scaled_inv_temp_percent = SCALE_TEMP_PERCENT(average_inv_module_temp, COOLING_INV_MAX_TEMP_C);
            int inv_duration_ms = scaled_inv_temp_percent * COOLING_TASK_PERIOD_MS;

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
        else
        {
            // No cooling needed / no temps read
            DEBUG_PRINT("Cooling disabled\n");
        }

        watchdogTaskCheckIn(COOLING_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, COOLING_TASK_PERIOD_MS);
    }
}
