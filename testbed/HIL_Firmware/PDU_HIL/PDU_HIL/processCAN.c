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
#include "digitalPot.h"

#define BYTE_0_MASK 0xFFFF00
#define BYTE_1_MASK 0xFF00FF
#define BYTE_2_MASK 0x00FFFF

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
            case BATTERY_THERMISTOR:
                byte_0 = can_msg.data[0];
                byte_1 = can_msg.data[1];
                byte_2 = can_msg.data[2];
                byte_0 |= BYTE_0_MASK;
                byte_1 = byte_1 << 8;
                byte_1 |= BYTE_1_MASK;
                byte_1 &= byte_0;
                byte_2 = byte_2 << 16;
                byte_2 |= BYTE_2_MASK;
                byte_2 &= byte_1;
                setPotResistance(byte_2);
                break;
            default:
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL_MS);
    }
}