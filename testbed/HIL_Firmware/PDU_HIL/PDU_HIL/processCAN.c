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

void process_rx_task (void * pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        xQueueReceive(rx_pdu_hil, &can_msg, portMAX_DELAY);

        switch (can_msg.identifier)
        {
            case 0x401030f: //Battery Thermistor
                dByte0 = can_msg.data[0];
                dByte1 = can_msg.data[1];
                dByte2 = can_msg.data[2];
                dByte0 |= 0b111111111111111100000000;
                dByte1 = dByte1 << 8;
                dByte1 |= 0b111111110000000011111111;
                dByte1 &= dByte0;
                dByte2 = dByte2 << 16;
                dByte2 |= 0b000000001111111111111111;
                dByte2 &= dByte1;
                setPotResitance(dByte2);
                break;
            default:
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL);
    }
}