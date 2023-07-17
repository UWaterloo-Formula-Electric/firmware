#include <stdio.h>
#include <stdlib.h>
#include "canReceive.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "driver/twai.h"

twai_message_t rx_msg;
twai_message_t can_msg;

QueueHandle_t rx_vcu_hil;
QueueHandle_t rx_pdu_hil;

void can_rx_task (void * pvParameters){

    rx_vcu_hil = xQueueCreate(MAX_QUEUE_LENGTH, sizeof(twai_message_t));
    rx_pdu_hil = xQueueCreate(MAX_QUEUE_LENGTH, sizeof(twai_message_t));

    TickType_t xLastWakeTime = xTaskGetTickCount();

    BaseType_t error;
    
    while(1) {

        if(twai_receive(&rx_msg,portMAX_DELAY)!= ESP_OK)
        {
            printf("failed to receive message\n");
        }
        
        #ifdef VCU_HIL_ID
            error = xQueueSend(rx_vcu_hil, &rx_msg, portMAX_DELAY);
            if(error != pdPASS)
            {
                printf("failed to send message to VCU HIL\n");
            }
        #endif
        
        #ifdef PDU_HIL_ID
            error = xQueueSend(rx_pdu_hil, &rx_msg, portMAX_DELAY);
            if(error != pdPASS)
            {
                printf("failed to send message to PDU HIL\n");
            }
        #endif

        vTaskDelayUntil(&xLastWakeTime, CAN_RX_TASK_INTERVAL);
    }
}