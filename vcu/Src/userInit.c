#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"
#include "drive_by_wire.h"
#include "userCan.h"
#include "drive_by_wire_mock.h"
#include "motorController.h"
#include "generalErrorHandler.h"
#include "vcu_F7_can.h"

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

    if (driveByWireInit() != HAL_OK)
    {
        Error_Handler();
    }

    uartStartReceiving(&DEBUG_UART_HANDLE);

    if (canInit(&CAN_HANDLE) != HAL_OK) {
      Error_Handler();
    }

    if (stateMachineMockInit() != HAL_OK) {
      Error_Handler();
    }

    if (initMotorControllerSettings() != HAL_OK) {
      Error_Handler();
    }
}

