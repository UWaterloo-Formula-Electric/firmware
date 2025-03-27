#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"
#include "controlStateMachine.h"
#include "userCan.h"
#include "controlStateMachine_mock.h"

// This is declared with weak linkage in all Cube main.c files, and called
// before freeRTOS initializes and starts up
void userInit()
{
    /* Should be the first thing initialized, otherwise print will fail */
    if (debugInit() != HAL_OK) {
        Error_Handler();
    }


    if (canInit(&CAN_HANDLE) != HAL_OK) {
      Error_Handler();
    }

    uartStartReceiving(&DEBUG_UART_HANDLE);
}
