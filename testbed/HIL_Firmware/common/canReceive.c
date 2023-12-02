#include <stdio.h>
#include <stdlib.h>
#include "canReceive.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "driver/twai.h"

twai_message_t rx_msg;
twai_message_t can_msg;

QueueHandle_t vcu_hil_queue;
QueueHandle_t pdu_hil_queue;
QueueHandle_t bmu_hil_queue;

void can_rx_task (void * pvParameters){

    vcu_hil_queue = xQueueCreate(MAX_CAN_MSG_QUEUE_LENGTH, sizeof(twai_message_t));
    pdu_hil_queue = xQueueCreate(MAX_CAN_MSG_QUEUE_LENGTH, sizeof(twai_message_t));
    bmu_hil_queue = xQueueCreate(MAX_CAN_MSG_QUEUE_LENGTH, sizeof(twai_message_t));

    TickType_t xLastWakeTime = xTaskGetTickCount();

    BaseType_t error = pdPASS;
    
    while(1) {

        if(twai_receive(&rx_msg,portMAX_DELAY)!= ESP_OK)
        {
            printf("failed to receive message\n");
        }
        
        #ifdef VCU_HIL_ID
            error = xQueueSend(vcu_hil_queue, &rx_msg, portMAX_DELAY);
            if(error != pdPASS)
            {
                printf("failed to send message to VCU HIL\n");
            }
        #endif
        
        #ifdef PDU_HIL_ID
            error = xQueueSend(pdu_hil_queue, &rx_msg, portMAX_DELAY);
            if(error != pdPASS)
            {
                printf("failed to send message to PDU HIL\n");
            }
        #endif

        #ifdef BMU_HIL_ID
            error = xQueueSend(bmu_hil_queue, &rx_msg, portMAX_DELAY);
            if(error != pdPASS)
            {
                printf("failed to send message to BMU HIL\n");
            }

        vTaskDelayUntil(&xLastWakeTime, CAN_RX_TASK_INTERVAL_MS);
    }
}
