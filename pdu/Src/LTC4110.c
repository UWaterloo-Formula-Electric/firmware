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
                uint32_t curr_state = fsmGetState(&coolingFsmHandle);
                if(DC_DC_state == false && curr_state == COOL_STATE_ON){
                    fsmSendEvent(&coolingFsmHandle, COOL_EV_EM_DISABLE, portMAX_DELAY); 
                }
                DEBUG_PRINT("Switched to battery\n");
            }
            DC_DC_state = newDCDCState;
        }
        watchdogTaskCheckIn(5);
        vTaskDelay(POWER_TASK_INTERVAL_MS);
    }
}
