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
#include "dac.h"
#include "canReceive.h"
#include "processCAN.h"

static uint16_t dbyte1;
static uint16_t dbyte2;

void process_rx_task (void * pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        xQueueReceive(rx_vcu_hil, &can_msg, portMAX_DELAY);

        switch (can_msg.identifier)
        {
            case 134480401:     //Brake position
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= 0b0000000011111111;
                dbyte1 |= 0b1111111100000000;
                dbyte2 &= dbyte1;
                set6551Voltage(dbyte2, brakePos_ID);
                break;
            case 134414865:     //Brake pres raw
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= 0b0000000011111111;
                dbyte1 |= 0b1111111100000000;
                dbyte2 &= dbyte1;
                setDacVoltage(dbyte2);
                break;
            case 134283793:     //Throttle A 
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= 0b0000000011111111;
                dbyte1 |= 0b1111111100000000;
                dbyte2 &= dbyte1;
                set6551Voltage(dbyte2, throttleA_ID);
                break;
            case 134349329:     //Throttle B
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= 0b0000000011111111;
                dbyte1 |= 0b1111111100000000;
                dbyte2 &= dbyte1;
                set6551Voltage(dbyte2, throttleB_ID);
                break;
            case 134545937:     //Steer Raw
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= 0b0000000011111111;
                dbyte1 |= 0b1111111100000000;
                dbyte2 &= dbyte1;
                set6551Voltage(dbyte2, steerRaw_ID);
                break;
            default:
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL);
    }
}