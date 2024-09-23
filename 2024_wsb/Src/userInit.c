#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "bsp.h"
#include "debug.h"
#include "userCan.h"
#include "controlStateMachine_mock.h"
#include "sensors.h"

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    signed char *pcTaskName )
{
    HAL_GPIO_WritePin(FW_HEARTBEAT_GPIO_Port, FW_HEARTBEAT_Pin, GPIO_PIN_SET);
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

    // if (canInit(&CAN_HANDLE) != HAL_OK) {
    //   Error_Handler();
    // }
    
    // if (stateMachineMockInit() != HAL_OK) {
    //   Error_Handler();
    // }

    // if (sensors_init() != HAL_OK) {
    //   Error_Handler();
    // }


    printf("User init done\n");
}

