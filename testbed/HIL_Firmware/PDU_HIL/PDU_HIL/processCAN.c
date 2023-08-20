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

static uint32_t dByte0 = 0;
static uint32_t dByte1 = 0;
static uint32_t dByte2 = 0;

static uint32_t dByte0_mask = 0xFFFF00;
static uint32_t dByte1_mask = 0xFF00FF;
static uint32_t dByte2_mask = 0x00FFFF;

void process_rx_task (void * pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        xQueueReceive(pdu_hil_queue, &can_msg, portMAX_DELAY);

        switch (can_msg.identifier)
        {
            case BATTERY_THERMISTOR: //Battery Thermistor
                dByte0 = can_msg.data[0];
                dByte1 = can_msg.data[1];
                dByte2 = can_msg.data[2];
                dByte0 |= dByte0_mask;
                dByte1 = dByte1 << 8;
                dByte1 |= dByte1_mask;
                dByte1 &= dByte0;
                dByte2 = dByte2 << 16;
                dByte2 |= dByte2_mask;
                dByte2 &= dByte1;
                setPotResistance(dByte2);
                break;
            default:
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL);
    }
}