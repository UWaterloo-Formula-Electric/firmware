/**
  *****************************************************************************
  * @file    mainTaskEntry.c
  * @author  Justin Vuong
  * @brief   Module containing main task, which is the default task for all
  * boards. It currently blinks the debug LED to indicate the firmware is running
  *****************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"

#define MAIN_TASK_PERIOD 1000

void mainTaskFunction(void const * argument){
    DEBUG_PRINT("Starting up!!\n");

    for(;;) {
        // DEBUG_PRINT("Alive!\n");
        vTaskDelay(10000);
    };
}
