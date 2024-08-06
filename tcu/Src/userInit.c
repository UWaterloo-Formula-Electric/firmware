/**
  *****************************************************************************
  * @file    userInit.c
  * @author  Jacky Lim
  * @brief   Initialization before RTOS starts
  * @details Contains the userInit function, which is called before the RTOS
  * starts to allow the user to initialize modules or other things that must be
  * done before the RTOS starts
  ******************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "uwfe_debug.h"
#include "userCan.h"
#include "controlStateMachine_mock.h"

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    signed char *pcTaskName )
{
    printf("Stack overflow for task %s\n", pcTaskName);
}

// This is declared with weak linkage in all Cube main.c files, and called
// before freeRTOS initializes and starts up
void userInit()
{
    /* Should be the first thing initialized, otherwise print will fail */
    if (debugInit() != HAL_OK) {
        Error_Handler();
    }

    if (uartStartReceiving(&DEBUG_UART_HANDLE) != HAL_OK)
    {
        Error_Handler();
    }

    /* I am commenting this out for now. Until we think of a better solution to read all 
     * CAN signals, I will just copy what Andrew did (initialize CAN without using userCAN)
     */
    // if (canInit(&CAN_HANDLE) != HAL_OK) {
    //   Error_Handler();
    // }
    
    if (stateMachineMockInit() != HAL_OK) {
      Error_Handler();
    }

    printf("User init done\n");
}

