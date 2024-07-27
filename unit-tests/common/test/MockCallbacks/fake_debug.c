#include "fake_debug.h"
#include "stddef.h"
#include "queue.h"
#include "uwfe_debug.h"

QueueHandle_t printQueue;
QueueHandle_t uartRxQueue;

HAL_StatusTypeDef fake_debugInit()
{
    printQueue = xQueueCreate(PRINT_QUEUE_LENGTH, PRINT_QUEUE_STRING_SIZE);
    if (!printQueue)
    {
        return HAL_ERROR;
    }

    uartRxQueue = xQueueCreate(UART_RX_QUEUE_LENGTH, 1);
    if (!printQueue)
    {
        return HAL_ERROR;
    }
	return HAL_OK;
}
/*
void printTask(void *pvParameters)
{
    char buffer[PRINT_QUEUE_STRING_SIZE] = {0};

    for ( ;; )
    {
        if (xQueueReceive(printQueue, buffer, portMAX_DELAY) == pdTRUE)
        {
            int len = strlen(buffer);
            HAL_UART_Transmit(&DEBUG_UART_HANDLE, (uint8_t*)buffer, len, UART_PRINT_TIMEOUT);
        }
    }
}
*/

void fake_mock_init_debug() {
	fake_debugInit();
}
