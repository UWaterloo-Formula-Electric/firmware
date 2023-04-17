#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"
#include "main.h"
#include "dac.h"
#include "canReceive.h"

//https://www.freertos.org/a00116.html

void proccess_rx_task (void * pvParameters){

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1){

        xQueueReceive(rx_vcu_hil, &can_msg, portMAX_DELAY);

        switch (can_msg.identifier){
            case 134480401:     //Brake position
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= 0b0000000011111111;
                dbyte1 |= 0b1111111100000000;
                dbyte2 &= dbyte1;
                set6551Voltage(dbyte2, brakePos_ID);
                break;
            case 134414865:     //Brake pres raw
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= 0b0000000011111111;
                dbyte1 |= 0b1111111100000000;
                dbyte2 &= dbyte1;
                setDacVoltage(dbyte2);
                    break;
            case 134283793:     //Throttle A 
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= 0b0000000011111111;
                dbyte1 |= 0b1111111100000000;
                dbyte2 &= dbyte1;
                set6551Voltage(dbyte2, throttleA_ID);
                break;
            case 134349329:     //Throttle B
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= 0b0000000011111111;
                dbyte1 |= 0b1111111100000000;
                dbyte2 &= dbyte1;
                set6551Voltage(dbyte2, throttleB_ID);
                break;
            case 134545937:     //Steer Raw
                dbyte1 = can_msg.data[0];
                dbyte2 = can_msg.data[1];
                dbyte2 = dbyte2 << 8;
                dbyte2 |= 0b0000000011111111;
                dbyte1 |= 0b1111111100000000;
                dbyte2 &= dbyte1;
                set6551Voltage(dbyte2, steerRaw_ID);
            default:
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, PROCCESS_RX_TASK_INTERVAL);
    }
}

void taskRegister (void){

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

    if(xReturned != pdPASS){
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

    if(xReturned != pdPASS){
        printf("Failed to register proccess_rx_task to RTOS");
    }
}

void CAN_init (void){
        //Initialize configuration structures using macro initializers
    twai_general_config_t g_config = {
        .mode = TWAI_MODE_NORMAL, 
        .tx_io = GPIO_NUM_12, 
        .rx_io = GPIO_NUM_13,
        .clkout_io = TWAI_IO_UNUSED, 
        .bus_off_io = TWAI_IO_UNUSED,      
        .tx_queue_len = 50, 
        .rx_queue_len = 50,                          
        .alerts_enabled = TWAI_ALERT_NONE,  
        .clkout_divider = 0,        
        .intr_flags = ESP_INTR_FLAG_LEVEL1
        };
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    //Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        //return ESP_FAIL;
    }

    //Start TWAI driver
    if (twai_start() == ESP_OK) {
        printf("Driver started\n");
    } else {
        printf("Failed to start driver\n");
        //return ESP_FAIL;
    }
    
}

void app_main(void)
{
    spi_init();
    CAN_init();
    taskRegister();
}
