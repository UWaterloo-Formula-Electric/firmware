#include <stdio.h>
#include <stdlib.h>
#include "canReceive.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
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

        if(twai_receive(&rx_msg,portMAX_DELAY)!= ESP_OK){
            printf("failed to receive message\n");
        }
        //https://www.freertos.org/a00117.html
        //printf("CAN message once receive: %lu\n", rx_msg.identifier);
        error = xQueueSend(rx_vcu_hil, &rx_msg, portMAX_DELAY);
        if(error != pdPASS){
            printf("failed to send message to VCU HIL\n");
        }
        error = xQueueSend(rx_pdu_hil, &rx_msg, portMAX_DELAY);
        if(error != pdPASS){
            printf("failed to send message to PDU HIL\n");
        }

        vTaskDelayUntil(&xLastWakeTime, CAN_RX_TASK_INTERVAL);
    }
}