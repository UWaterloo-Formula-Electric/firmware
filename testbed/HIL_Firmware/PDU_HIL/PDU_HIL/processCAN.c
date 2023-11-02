#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "userInit.h"
#include "canReceive.h"
#include "processCAN.h"
#include "digitalPot.h"

static uint32_t byte_0 = 0U;
static uint32_t byte_1 = 0U;
static uint32_t byte_2 = 0U;

void process_rx_task (void * pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        xQueueReceive(pdu_hil_queue, &can_msg, portMAX_DELAY);

        switch (can_msg.identifier)
        {
            case BATTERY_THERMISTOR_CAN_ID:
                byte_0 = can_msg.data[0];
                byte_1 = can_msg.data[1];
                byte_2 = can_msg.data[2];
                byte_1 = byte_1 << 8;
                byte_1 |= byte_0;
                byte_2 = byte_2 << 16;
                byte_2 |= byte_1;
                setPotResistance(byte_2);
                break;
            default:
                printf("CAN ID not recognized %ld\r\n", can_msg.identifier);
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL_MS);
    }
}
