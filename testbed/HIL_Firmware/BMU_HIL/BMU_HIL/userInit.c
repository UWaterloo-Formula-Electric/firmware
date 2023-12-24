#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "canReceive.h"
#include "userInit.h"
#include "processCAN.h"
#include "bmuOutputs.h"

/* Registering all tasks for FreeRTOS */
void taskRegister(void)
{
    BaseType_t xReturned = pdPASS;
    TaskHandle_t can_rx_task_handler = NULL;
    TaskHandle_t can_process_task_handler = NULL;
    TaskHandle_t relay_bmu_outputs_handler = NULL;

    /* Set up handler for receiving CAN messages */
    xReturned = xTaskCreate(
        can_rx_task,
        "CAN_RECEIVE_TASK",
        4000,
        ( void * ) NULL,
        configMAX_PRIORITIES-1,
        &can_rx_task_handler
    );

    if(xReturned != pdPASS)
    {
        while(1)
        {
            printf("Failed to register can_rx_task to RTOS");
        }
    }

    /* Set up handler for processing CAN messages */
    xReturned = xTaskCreate(
        process_rx_task,
        "CAN_PROCESS_TASK",
        4000,
        ( void * ) NULL,
        configMAX_PRIORITIES-1,
        &can_process_task_handler
    );

    if(xReturned != pdPASS)
    {
        while(1)
        {
            printf("Failed to register process_rx_task to RTOS");
        }
    }

    /* Set up handler for relaying BMU outputs */
    xReturned = xTaskCreate(
        relayBmuOutputs,
        "RELAY_BMU_OUTPUTS_TASK",
        4000,
        ( void * ) NULL,
        configMAX_PRIORITIES-1,
        &relay_bmu_outputs_handler
    );

    if(xReturned != pdPASS)
    {
        while(1)
        {
            printf("Failed to register relayBmuOutputs to RTOS");
        }
    }
}

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
        .tx_io = CAN_TX_VEH, 
        .rx_io = CAN_RX_VEH,
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

void bmu_input_init(void)
{
    gpio_set_direction(FAN_PWM_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(FAN_TACH_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CS_HV_NEG_OUTPUT_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CS_HV_POS_OUTPUT_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(IMD_STATUS_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(IMD_FAULT_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(HVD_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(IL_CLOSE_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(TSMS_FAULT_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CBRB_PRESS_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CS_HV_SHUNT_NEG_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CS_HV_SHUNT_POS_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CS_BATT_NEG_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CS_BATT_POS_PIN, GPIO_MODE_INPUT);
}

void app_main(void)
{
    CAN_init();
    bmu_input_init();
    taskRegister();
}