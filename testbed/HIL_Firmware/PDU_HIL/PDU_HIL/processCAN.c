#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"
#include "userInit.h"
#include "canReceive.h"
#include "processCAN.h"

void process_rx_task (void * pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        xQueueReceive(rx_pdu_hil, &can_msg, portMAX_DELAY);

        switch (can_msg.identifier)
        {
            case 1234:
                break;
            default:
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL);
    }
}