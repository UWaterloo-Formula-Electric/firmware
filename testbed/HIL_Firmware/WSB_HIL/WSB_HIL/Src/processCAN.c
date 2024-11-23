#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/twai.h"
#include "../Inc/userInit.h"
#include "dac.h"
#include "canReceive.h"
#include "../Inc/processCAN.h"

void process_rx_task (void * pvParamters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1) {
        xQueueReceive(wsb_hil_queue, &can_msg, portMAX_DELAY);

        switch(can_msg.identifier) {
            case WSBFL_CAN_ID:
                break;
            case WSBFR_CAN_ID:
                break;
            case WSBRL_CAN_ID:
                break;
            case WSBRR_CAN_ID:
                break;
            default:
                printf("CAN ID not recognized %ld\r\n", can_msg.identifier);
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL_MS);
    }
}