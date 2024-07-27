#include "user_tcp.h"

#include "uwfe_debug.h"

#define TCP_TASK_PERIOD_MS 100

void tcpTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TCP_TASK_PERIOD_MS));
    }
}