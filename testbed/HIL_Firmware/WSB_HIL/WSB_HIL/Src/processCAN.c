#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/twai.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "../Inc/userInit.h"
#include "../Inc/dac.h"
#include "canReceive.h"
#include "../Inc/processCAN.h"

//reassembles data into 16 bit uint to pass in desired values to dac & pwm functions
void process_rx_task (void * pvParamters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1) {
        xQueueReceive(wsb_hil_queue, &can_msg, portMAX_DELAY);

        switch(can_msg.identifier) {
            case WSBFL_CAN_ID:
                processFL(&can_msg);
                break;
            case WSBFR_CAN_ID:
                processFR(&can_msg);
                break;
            case WSBRL_CAN_ID:
                processRL(&can_msg);
                break;
            case WSBRR_CAN_ID:
                processRR(&can_msg);
                break;
            default:
                printf("CAN ID not recognized %ld\r\n", can_msg.identifier);
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL_MS);
    }
}

static uint8_t bytes[8];
static uint16_t brake_ir_voltage = 0U;
static uint16_t hall_effect_freq = 0U;

esp_err_t processFL(twai_message_t* can_msg) {
    input_data(can_msg, WSBFL_MSG_LENGTH);

    brake_ir_voltage = bytes[1] << 8;
    brake_ir_voltage |= bytes[0];

    if (set_dac_voltage(&brake_ir_handle, brake_ir_voltage) != ESP_OK) {
        printf("brake ir voltage not set");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t processFR(twai_message_t* can_msg) {
    input_data(can_msg, WSBFR_MSG_LENGTH);

    return ESP_OK;
}

esp_err_t processRL(twai_message_t* can_msg) {
    input_data(can_msg, WSBRL_MSG_LENGTH);

    hall_effect_freq = bytes[0];


    if (ledc_set_freq(LEDC_LOW_SPEED_MODE, PwmTimer_HallEff, hall_effect_freq) != ESP_OK) {
        printf("hall effect speed not set");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t processRR(twai_message_t* can_msg) {
    input_data(can_msg, WSBRR_MSG_LENGTH);

    brake_ir_voltage = bytes[1] << 8;
    brake_ir_voltage |= bytes[0];
    hall_effect_freq = bytes[2];

    if (set_dac_voltage(&brake_ir_handle, brake_ir_voltage) != ESP_OK) {
        printf("brake ir voltage not set");
        return ESP_FAIL;
    }

    if (ledc_set_freq(LEDC_LOW_SPEED_MODE, PwmTimer_HallEff, hall_effect_freq) != ESP_OK) {
        printf("hall effect speed not set");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void input_data(twai_message_t* can_msg, uint8_t msg_length) {
    if (msg_length > 8) {
        printf("Message length too long");
        return;
    }

    for (int i = 0; i < msg_length; ++i) {
        bytes[i] = can_msg->data[i];
    }
}