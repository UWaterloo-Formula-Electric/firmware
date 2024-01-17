#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "userInit.h"
#include "canReceive.h"
#include "processCAN.h"
#include "pwm.h"

/* Variables needed for CAN message processing */
static uint32_t byte_0 = 0U;
static uint32_t byte_1 = 0U;
static uint32_t byte_2 = 0U;

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

            default:
                printf("CAN ID not recognized %ld\r\n", can_msg.identifier);
                break;
        }

        // FreeRTOS function that delays the task until the next interval
        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL_MS);
    }
}