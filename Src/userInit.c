#include "freertos.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"
#include "drive_by_wire.h"

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    signed char *pcTaskName )
{
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    printf("Stack overflow for task %s\n", pcTaskName);
}


// This is declared with weak linkage in all Cube main.c files, and called
// before freeRTOS initializes and starts up
void userInit()
{
    if (driveByWireInit() != HAL_OK)
    {
        Error_Handler();
    }

    uartStartReceiving(&DEBUG_UART_HANDLE);
}

