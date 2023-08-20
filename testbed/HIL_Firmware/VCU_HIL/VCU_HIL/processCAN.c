#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/twai.h"
#include "userInit.h"
#include "dac.h"
#include "canReceive.h"
#include "processCAN.h"

static uint16_t dbyte1;
static uint16_t dbyte2;

//reassemble data into a 16 bit uint to pass in desired voltage into set dac function
static uint16_t dbyte2_mask = 0x00FF;
static uint16_t dbyte1_mask = 0xFF00;

void process_rx_task (void * pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        xQueueReceive(vcu_hil_queue, &can_msg, portMAX_DELAY);

        switch (can_msg.identifier)
        {
            case BRAKE_POS:     //Brake position
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= dbyte2_mask;
                dbyte1 |= dbyte1_mask;
                dbyte2 &= dbyte1;
                set6551Voltage(dbyte2, brakePos_ID);
                break;
            case BRAKE_PRES_RAW:     //Brake pres raw
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= dbyte2_mask;
                dbyte1 |= dbyte1_mask;
                dbyte2 &= dbyte1;
                setDacVoltage(dbyte2);
                break;
            case THROTTLE_A:     //Throttle A 
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= dbyte2_mask;
                dbyte1 |= dbyte1_mask;
                dbyte2 &= dbyte1;
                set6551Voltage(dbyte2, throttleA_ID);
                break;
            case THROTTLE_B:     //Throttle B
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= dbyte2_mask;
                dbyte1 |= dbyte1_mask;
                dbyte2 &= dbyte1;
                set6551Voltage(dbyte2, throttleB_ID);
                break;
            case STEER_RAW:     //Steer Raw
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= dbyte2_mask;
                dbyte1 |=dbyte1_mask;
                dbyte2 &= dbyte1;
                set6551Voltage(dbyte2, steerRaw_ID);
                break;
            default:
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL);
    }
}