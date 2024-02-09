#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "userInit.h"
#include "canReceive.h"
#include "processCAN.h"
#include "pwm.h"
#include "pwm.h"

/* Variables needed for CAN message processing */
static uint32_t dbyte1 = 0U;
static uint32_t dbyte2 = 0U;
static uint32_t dbyte3 = 0U;
static uint32_t dbyte4 = 0U;

static const uint32_t dbyte1_mask = 0xFFFFFF00;
static const uint32_t dbyte2_mask = 0xFFFF00FF;
static const uint32_t dbyte3_mask = 0xFF00FFFF;
static const uint32_t dbyte4_mask = 0x00FFFFFF;

/* Processes incoming CAN message */
void process_rx_task (void * pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1)
    {

        // FreeRTOS function that receives a CAN message from the queue
        xQueueReceive(bmu_hil_queue, &can_msg, portMAX_DELAY);
        
        switch(can_msg.identifier)
        {
            case BMU_PWM_CONFIG_CAN_ID:
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= dbyte2_mask;
                dbyte1 |= dbyte1_mask;
                const uint16_t PWMDutyCycle = dbyte2 & dbyte1;

                dbyte1 = can_msg.data[2];
                dbyte1 |= dbyte1_mask;
                dbyte2 = can_msg.data[3];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= dbyte2_mask;
                dbyte3 = can_msg.data[4];
                dbyte3 = dbyte3 << 16;
                dbyte3 |= dbyte3_mask;
                dbyte4 = can_msg.data[5];
                dbyte4 = dbyte4 << 24;
                dbyte4 |= dbyte4_mask;
                const uint32_t PWMFreqHz = dbyte4 & dbyte3 & dbyte2 & dbyte1;

                const uint8_t PWMPinNumber = can_msg.data[6];
                pwm_set_pin(PWM_CHANNEL_INDEX_IMD, PWMPinNumber, PWMFreqHz, PWMDutyCycle);
                break;

            default:
                printf("CAN ID not recognized %ld\r\n", can_msg.identifier);
                break;
        }

        // FreeRTOS function that delays the task until the next interval
        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL_MS);
    }
}