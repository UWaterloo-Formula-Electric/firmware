/**
  *****************************************************************************
  * @file    mainTaskEntry.c
  * @brief   Module containing main task, which is the default task for all
  * boards. It starts the CAN bus, then blinks the debug LED to indicate the
  * firmware is running.
  *****************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"
#include "userCan.h"

#define MAIN_TASK_PERIOD 1000

void mainTaskFunction(void const * argument)
{
    DEBUG_PRINT("Starting up!!\n");

    // Takes the bus out of init mode and enables the RX FIFO interrupts.
    // Deferred to task context, the same as every other board does it.
    if (canStart(&CAN_HANDLE) != HAL_OK) {
        ERROR_PRINT("Failed to start CAN!\n");
        Error_Handler();
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        HAL_GPIO_TogglePin(DEBUG_LED_PORT, DEBUG_LED_PIN);

        vTaskDelayUntil(&xLastWakeTime, MAIN_TASK_PERIOD);
    }
}
