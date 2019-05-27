#include "LTC4110.h"
#include "bsp.h"
#include "freertos.h"
#include "task.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "FreeRTOS_CLI.h"
#include "debug.h"
#include "adc.h"
#include <stdbool.h>
#include "PDU_can.h"
#include "watchdog.h"

void powerTask(void *pvParameters)
{
    if (registerTaskToWatch(5, 2*POWER_TASK_INTERVAL_MS, false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register power task with watchdog!\n");
        Error_Handler();
    }

    // Delay to allow system to turn on
    vTaskDelay(100);

    bool lastDCState = GET_DC_PRESNT;
    bool newDCState = GET_DC_PRESNT;
    while (1)
    {
       
        if (lastDCState != newDCState)
        {
            if (newDCState == GPIO_PIN_SET) 
            {
                DEBUG_PRINT("switched to DC to DC\n");
            }
            else
            {
                DEBUG_PRINT("switched to DC to DC\n");
            }
        }
        watchdogTaskCheckIn(5);
        vTaskDelay(POWER_TASK_INTERVAL_MS);
    }
}

BaseType_t printPowerStates(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{

    COMMAND_OUTPUT("States:\n DC not present:%d\n", GET_DC_PRESNT);
    return pdFALSE;
}
static const CLI_Command_Definition_t printPowerStatesCommandDefinition =
{
    "powerStates",
    "powerStates:\r\n  Output current states of LTC4110\r\n",
    printPowerStates,
    0 /* Number of parameters */
};


HAL_StatusTypeDef LTC4110Init()
{
    if (FreeRTOS_CLIRegisterCommand(&printPowerStatesCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    return HAL_OK;
}




 // GET_CHARGE_STATUS 
 // GET_BATT_UV_STATUS
 // GET_CAL_CMPLT  
 // GET_DC_PRESNT  
 // DC_POWER_DISABLE 
 // DC_POWER_ENABLE 