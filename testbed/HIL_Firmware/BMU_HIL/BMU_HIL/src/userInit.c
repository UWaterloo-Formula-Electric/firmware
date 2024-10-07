/***********************************
************ INCLUDES **************
************************************/

// Standard Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ESP-IDF Includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "driver/spi_master.h"

// Inter-Component Includes
#include "canReceive.h"

// Inta-Component Includes
#include "userInit.h"


/***********************************
***** FUNCTION DEFINITIONS *********
************************************/
esp_err_t CAN_init (void)
{
    memset(&rx_msg, 0, sizeof(twai_message_t));
    memset(&can_msg, 0, sizeof(twai_message_t));
    memset(&bmu_hil_queue, 0, sizeof(QueueHandle_t)); //not 0 ing other queues to avoid repetition

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

esp_err_t SPI_init(void)
{
    return ESP_OK;
}

esp_err_t GPIO_init (void)
{
    return ESP_OK;
}

void app_main(void)
{
    CAN_init();
    SPI_init();
    GPIO_init();
}