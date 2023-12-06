#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_err.h"
#include "userInit.h"
#include "processCAN.h"
#include "canReceive.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"

void taskRegister(void) {
    BaseType_t xReturned;
    TaskHandle_t can_rx_handle;
    TaskHandle_t process_rx_handle;

    xReturned = xTaskCreate(
        can_rx_task;
        "CAN_RECEIVE_TASK";
        // stack size.
        (void *) 1;
        // priority.
        &can_rx_handle;
    )

    if (xReturned != pdPass) {
        printf("Failed to register can_rx_task to FreeRTOS");
    }

    xReturned = xTaskCreate(
        process_rx_task; // define in processCAN.c
        "CAN_PROCESS_TASK";
        // stack size.
        (void *) 1;
        // priority.
        &process_rx_handle;
    )

    if (xReturned != pdPass) {
        printf("Failed to register process_rx_task to FreeRTOS.");
    }
    
    return;
}

esp_err_t CAN_init(void) {
    twai_general_config_t g_config = { // https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/api-reference/peripherals/twai.html#_CPPv421twai_general_config_t
        .mode = TWAI_MODE_NORMAL,
        .tx_io = CAN_TX, // define in userInit.h
        .rx_io = CAN_RX,
        .clkout_io = TWAI_IO_UNUSED, 
        .bus_off_io = TWAI_IO_UNUSED,      
        .tx_queue_len = MAX_CAN_MSG_QUEUE_LENGTH, // check if supposed to be this > MAX_QUEUE_LENGTH
        .rx_queue_len = MAX_CAN_MSG_QUEUE_LENGTH,                          
        .alerts_enabled = TWAI_ALERT_NONE,  
        .clkout_divider = 0,        
        .intr_flags = ESP_INTR_FLAG_LEVEL1
    };
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("TWAI driver installed.");
    } else {
        printf("TWAI driver failed to install.");
        return ESP_FAIL;
    }

    if (twai_start() == ESP_OK) {
        printf("TWAI driver started.");
    } else {
        printf("TWAI driver failed to start.");
        return ESP_FAIL;
    }
}

// pin arrays
const int OUTPUT_PIN_ARRAY[] = {

};

// esp_err_t spi_init(void) { // check if this is needed
//     return ESP_OK;
// }

void app_main(void) {
    taskRegister();
    CAN_init();
    // spi_init();
}