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
    if (registerTaskToWatch(POWER_TASK_ID, 2*POWER_TASK_INTERVAL_MS, false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register power task with watchdog!\n");
        Error_Handler();
    }

    // Delay to allow system to turn on
    vTaskDelay(100);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        bool newDCDCState = CHECK_DC_DC_ON_PIN;

        if (newDCDCState != DC_DC_state) {
            if (newDCDCState)
            {
                DEBUG_PRINT("switched to DC to DC\n");
            }
            DC_DC_state = newDCDCState;

            sendCAN_PDU_DCDC_Status();
        }
        watchdogTaskCheckIn(POWER_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, POWER_TASK_INTERVAL_MS);
    }
}