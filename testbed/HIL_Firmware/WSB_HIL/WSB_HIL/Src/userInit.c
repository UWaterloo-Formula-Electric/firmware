#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "driver/ledc.h"
#include "driver/dac_oneshot.h"
#include "../Inc/userInit.h"
#include "../Inc/dac.h"
#include "canReceive.h"
#include "../Inc/processCAN.h"

dac_oneshot_handle_t brake_ir_handle;

void taskRegister (void) {
    BaseType_t xReturned = pdPASS;
    TaskHandle_t can_rx;
    TaskHandle_t can_process;

    xReturned = xTaskCreate(
        can_rx_task,
        "CAN_RECEIVE_TASK",
        4000,
        ( void * ) NULL,
        configMAX_PRIORITIES-1,
        &can_rx
    );

    if(xReturned != pdPASS)
    {
        while(1)
        {
            printf("Failed to register can_rx_task to RTOS\r\n");
        }
    }

    xReturned = xTaskCreate(
        process_rx_task,
        "CAN_PROCESS_TASK",
        4000,
        ( void * ) NULL,
        configMAX_PRIORITIES-1,
        &can_process
    );

    if(xReturned != pdPASS)
    {
        while(1)
        {
            printf("Failed to register process_rx_task to RTOS\r\n");
        }
    }
}

esp_err_t CAN_init (void) {

    memset(&rx_msg, 0, sizeof(twai_message_t));
    memset(&can_msg, 0, sizeof(twai_message_t));
    memset(&vcu_hil_queue, 0, sizeof(QueueHandle_t));
    memset(&pdu_hil_queue, 0, sizeof(QueueHandle_t));
    memset(&wsb_hil_queue, 0, sizeof(QueueHandle_t));

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

esp_err_t dac_init(void) {
    memset(&brake_ir_handle, 0, sizeof(dac_oneshot_handle_t));

    dac_oneshot_config_t brake_ir = {
            .chan_id = DAC_CHAN_BRAKE_IR,
    };

    if (dac_oneshot_new_channel(&brake_ir, &brake_ir_handle) != ESP_OK) {
        printf("failed to add brakeir DAC to bus");
        return ESP_FAIL;
    }

    if (set_dac_voltage(&brake_ir_handle, 0) != ESP_OK) {
        printf("failed to set DAC");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t pwm_init() {
    ledc_timer_config_t hall_effect = {
            .speed_mode       = LEDC_LOW_SPEED_MODE,
            .timer_num        = PwmTimer_HallEff,
            .duty_resolution  = LEDC_TIMER_8_BIT,
            .freq_hz          = 0,
            .clk_cfg          = LEDC_AUTO_CLK
    };

    ledc_channel_config_t hall_effect_channel = {
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = PwmChannel_HallEff,
            .timer_sel      = PwmTimer_HallEff,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = HL_EFF,
            .duty           = LEDC_TIMER_8_BIT/2,
            .hpoint         = 0
    };

    if (ledc_timer_config(&hall_effect) != ESP_OK) {
        printf("failed to initialize hall effect pwm timer");
        return ESP_FAIL;
    }

    if (ledc_channel_config(&hall_effect_channel) != ESP_OK) {
        printf("failed to initialize hall effect pwm channel");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void app_main(void)
{
    dac_init();
    pwm_init();
    CAN_init();
    taskRegister();
}