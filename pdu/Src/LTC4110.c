#include "LTC4110.h"
#include "bsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "FreeRTOS_CLI.h"
#include "debug.h"
#include "adc.h"
#include <stdbool.h>
#include "pdu_can.h"
#include "watchdog.h"
#include "pdu_dtc.h"

volatile bool DC_DC_state = false;

void powerTask(void *pvParameters)
{
    if (registerTaskToWatch(5, 2*POWER_TASK_INTERVAL_MS, false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register power task with watchdog!\n");
        Error_Handler();
    }

    // Delay to allow system to turn on
    vTaskDelay(100);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        bool newDCDCState = IS_DC_DC_ON;

        if (newDCDCState != DC_DC_state) {
            if (newDCDCState)
            {
                DEBUG_PRINT("switched to DC to DC\n");
            }
            else
            {
                if(fsmGetState(&coolingFsmHandle) == COOL_STATE_ON){
                    fsmSendEvent(&motorFsmHandle, COOL_EV_EM_DISABLE, portMAX_DELAY);
                }
                if(fsmGetState(&motorFsmHandle) == MTR_STATE_Motors_On){
                    fsmSendEvent(&motorFsmHandle, MTR_EV_EM_DISABLE, portMAX_DELAY);
                }
                sendDTC_ERROR_DCDC_Shutoff();
                DEBUG_PRINT("Switched to battery\n");
            }
            DC_DC_state = newDCDCState;
        }
        watchdogTaskCheckIn(5);
        vTaskDelayUntil(&xLastWakeTime, POWER_TASK_INTERVAL_MS);
    }
}
