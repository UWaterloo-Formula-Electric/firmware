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

#define BYTE_1_MASK 0xFF00
#define BYTE_2_MASK 0x00FF

static uint16_t byte_1;
static uint16_t byte_2;

//reassemble data into a 16 bit uint to pass in desired voltage into set dac function
void process_rx_task (void * pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        xQueueReceive(vcu_hil_queue, &can_msg, portMAX_DELAY);

        switch (can_msg.identifier)
        {
            case BRAKE_POS:     //Brake position
                byte_1 = can_msg.data[0];
                byte_2 = can_msg.data[1];
                byte_2 = byte_2 << 8;
                byte_2 |= BYTE_2_MASK;
                byte_1 |= BYTE_1_MASK;
                byte_2 &= byte_1;
                set6551Voltage(byte_2, DacId_brakePos);
                break;
            case BRAKE_PRES_RAW:     //Brake pres raw
                byte_1 = can_msg.data[0];
                byte_2 = can_msg.data[1];
                byte_2 = byte_2 << 8;
                byte_2 |= BYTE_2_MASK;
                byte_1 |= BYTE_1_MASK;
                byte_2 &= byte_1;
                setDacVoltage(byte_2);
                break;
            case THROTTLE_A:     //Throttle A 
                byte_1 = can_msg.data[0];
                byte_2 = can_msg.data[1];
                byte_2 = byte_2 << 8;
                byte_2 |= BYTE_2_MASK;
                byte_1 |= BYTE_1_MASK;
                byte_2 &= byte_1;
                set6551Voltage(byte_2, DacId_throttleA);
                break;
            case THROTTLE_B:     //Throttle B
                byte_1 = can_msg.data[0];
                byte_2 = can_msg.data[1];
                byte_2 = byte_2 << 8;
                byte_2 |= BYTE_2_MASK;
                byte_1 |= BYTE_1_MASK;
                byte_2 &= byte_1;
                set6551Voltage(byte_2, DacId_throttleB);
                break;
            case STEER_RAW:     //Steer Raw
                byte_1 = can_msg.data[0];
                byte_2 = can_msg.data[1];
                byte_2 = byte_2 << 8;
                byte_2 |= BYTE_2_MASK;
                byte_1 |= BYTE_1_MASK;
                byte_2 &= byte_1;
                set6551Voltage(byte_2, DacId_steerRaws);
                break;
            default:
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL_MS);
    }
}