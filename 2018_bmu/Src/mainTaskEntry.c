/**
  *****************************************************************************
  * @file    mainTaskEntry.c
  * @author  Richard Matthews
  * @brief   Module containing main task, which is the default task for all
  * boards. It currently blinks the debug LED to indicate the firmware is running
  *****************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"

void mainTaskFunction(void const * argument)
{
    DEBUG_PRINT("Starting up!!\n");
    while (1) {
        HAL_GPIO_TogglePin(DEBUG_LED_PORT, DEBUG_LED_PIN);

        vTaskDelay(1000);
    }
}
