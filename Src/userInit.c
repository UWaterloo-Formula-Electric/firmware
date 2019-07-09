#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"
#include "userCan.h"
#include "controlStateMachine_mock.h"
#include "controlStateMachine.h"
#include "batteries.h"

#if IS_BOARD_F7
#include "imdDriver.h"
#endif

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    signed char *pcTaskName )
{
    HAL_GPIO_WritePin(ERROR_LED_PORT, ERROR_LED_PIN, GPIO_PIN_SET);
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

    if (canInit(&CAN_HANDLE) != HAL_OK) {
      Error_Handler();
    }

    if (stateMachineMockInit() != HAL_OK) {
        Error_Handler();
    }
    if (controlInit() != HAL_OK) {
        Error_Handler();
    }
    if (initBusVoltagesAndCurrentQueues() != HAL_OK) {
        Error_Handler();
    }
    if (initPackVoltageQueue() != HAL_OK) {
        Error_Handler();
    }
#if IS_BOARD_F7
    if (init_imd_measurement() != HAL_OK) {
        Error_Handler();
    }
#endif

    printf("Finished user init\n");
}

