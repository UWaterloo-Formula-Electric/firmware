#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h" // ??
#include "driver/gpio.h"
#include "driver/twai.h"
#include "canReceive.h"
#include "userInit.h"
#include "processCAN.h"

#define CAN_TX GPIO_NUM_36
#define CAN_RX GPIO_NUM_35

esp_err_t CAN_init (void)
{
    /* The following lines clear all memory blocks to 0 to ensure that we start clean
       before we initialize CAN */
    memset(&rx_msg, 0, sizeof(twai_message_t));
    memset(&can_msg, 0, sizeof(twai_message_t));
    memset(&vcu_hil_queue, 0, sizeof(QueueHandle_t));
    memset(&pdu_hil_queue, 0, sizeof(QueueHandle_t));
    memset(&bmu_hil_queue, 0, sizeof(QueueHandle_t));

    twai_general_config_t g_config = {
        .mode = TWAI_MODE_NORMAL, 
        .tx_io = CAN_TX, 
        .rx_io = CAN_RX,
        .clkout_io = TWAI_IO_UNUSED, 
        .bus_off_io = TWAI_IO_UNUSED,      
        .tx_queue_len = MAX_CAN_MSG_QUEUE_LENGTH, 
        .rx_queue_len = MAX_CAN_MSG_QUEUE_LENGTH,                          
        .alerts_enabled = TWAI_ALERT_NONE,  
        .clkout_divider = 0,        
        .intr_flags = ESP_INTR_FLAG_LEVEL1
        };

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK)
    {
        printf("Failed to install TWAI driver\r\n");
        return ESP_FAIL;
    } 
    printf("TWAI driver installed\r\n");

    if (twai_start() != ESP_OK) 
    {
        printf("Failed to start TWAI driver\r\n");
        return ESP_FAIL;
        
    } 

    printf("TWAI driver started\r\n");
    return ESP_OK;
}

void app_main()
{
    CAN_init();
}