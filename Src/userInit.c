#include "freertos.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"
#include "userCan.h"

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
    if (canInit(&CAN_HANDLE) != HAL_OK) {
      Error_Handler();
    }
    uartStartReceiving(&DEBUG_UART_HANDLE);
}

