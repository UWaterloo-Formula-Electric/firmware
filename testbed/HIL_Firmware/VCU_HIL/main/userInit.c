#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"
#include "userInit.h"
#include "dac.h"
#include "canReceive.h"
#include "proccessCAN.h"

//https://www.freertos.org/a00116.html

void taskRegister (void)
{
    BaseType_t xReturned;
    TaskHandle_t can_rx;
    TaskHandle_t can_proccess;

    xReturned = xTaskCreate(
        can_rx_task,
        "CAN_RECEIVE_TASK",
        4000,
        ( void * ) 1,
        configMAX_PRIORITIES-1,
        &can_rx
    );

    if(xReturned != pdPASS)
    {
        printf("Failed to register can_rx_task to RTOS");
    }

    xReturned = xTaskCreate(
        proccess_rx_task,
        "CAN_PROCCESS_TASK",
        4000,
        ( void * ) 1,
        configMAX_PRIORITIES-1,
        &can_proccess
    );

    if(xReturned != pdPASS)
    {
        printf("Failed to register proccess_rx_task to RTOS");
    }
}

int CAN_init (void)
{
     //Initialize configuration structures using macro initializers
    twai_general_config_t g_config = {
        .mode = TWAI_MODE_NORMAL, 
        .tx_io = GPIO_NUM_12, 
        .rx_io = GPIO_NUM_13,
        .clkout_io = TWAI_IO_UNUSED, 
        .bus_off_io = TWAI_IO_UNUSED,      
        .tx_queue_len = MAX_QUEUE_LENGTH, 
        .rx_queue_len = MAX_QUEUE_LENGTH,                          
        .alerts_enabled = TWAI_ALERT_NONE,  
        .clkout_divider = 0,        
        .intr_flags = ESP_INTR_FLAG_LEVEL1
        };
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    //Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        printf("Driver installed\n");
    } 
    else 
    {
        printf("Failed to install driver\n");
        return ESP_FAIL;
    }

    //Start TWAI driver
    if (twai_start() == ESP_OK) 
    {
        printf("Driver started\n");
        return ESP_OK;
    } 
    else 
    {
        printf("Failed to start driver\n");
        return ESP_FAIL;
    }
    
}

void app_main(void)
{
    spi_init();
    CAN_init();
    taskRegister();
}
