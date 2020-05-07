/**
  *****************************************************************************
  * @file    userInit.c
  * @author  Richard Matthews
  * @brief   Initialization before RTOS starts
  * @details Contains the userInit function, which is called before the RTOS
  * starts to allow the user to initialize modules or other things that must be
  * done before the RTOS starts
  *
  ******************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"
#include "userCan.h"
#include "DCU_can.h"

/**
 * Called by FreeRTOS on stack overflow
 */
void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    signed char *pcTaskName )
{
    /*HAL_GPIO_WritePin(ERROR_LED_PORT, ERROR_LED_PIN, GPIO_PIN_SET);*/
    printf("Stack overflow for task %s\n", pcTaskName);
}


/**
 * Perform initialization before RTOS starts
 * This is declared with weak linkage in all Cube main.c files, and called
 * before freeRTOS initializes and starts up
 */
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

    if (canInit(&CAN_HANDLE) != HAL_OK) {
      Error_Handler();
    }

    HV_Power_State = 0;
    EM_State = 0;

    printf("User init done\n");
}

