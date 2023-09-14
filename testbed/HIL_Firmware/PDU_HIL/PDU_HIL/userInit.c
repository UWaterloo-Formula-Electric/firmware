#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "driver/spi_master.h"
#include "canReceive.h"
#include "userInit.h"
#include "processCAN.h"
#include "digitalPot.h"
#include "pduOutputs.h"

spi_device_handle_t pot;

void taskRegister (void)
{
    BaseType_t xReturned = pdPASS;
    TaskHandle_t can_rx_task_handler = NULL;
    TaskHandle_t can_process_task_handler = NULL;
    TaskHandle_t relay_pdu_outputs_handler = NULL;

    xReturned = xTaskCreate(
        can_rx_task,
        "CAN_RECEIVE_TASK",
        4000,
        ( void * ) 1,
        configMAX_PRIORITIES-1,
        &can_rx_task_handler
    );

    if(xReturned != pdPASS)
    {
        printf("Failed to register can_rx_task to RTOS");
    }

    xReturned = xTaskCreate(
        process_rx_task,
        "CAN_PROCESS_TASK",
        4000,
        ( void * ) 1,
        configMAX_PRIORITIES-1,
        &can_process_task_handler
    );

    if(xReturned != pdPASS)
    {
        printf("Failed to register process_rx_task to RTOS");
    }

    xReturned = xTaskCreate(
        relayPduOutputs,
        "RELAY_PDU_OUTPUTS_TASK",
        4000,
        ( void * ) 1,
        configMAX_PRIORITIES-1,
        &relay_pdu_outputs_handler
    );

    if(xReturned != pdPASS)
    {
        printf("Failed to register relayPduOutputs to RTOS");
    }
}

int CAN_init (void)
{
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

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        printf("Driver installed\n");
    } 
    else 
    {
        printf("Failed to install driver\n");
        return ESP_FAIL;
    }

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

int spi_init(void)
{
    printf("Initializing SPI bus\n");

    memset(&pot, 0, sizeof(spi_device_handle_t));

    esp_err_t ret;

    spi_bus_config_t buscfg={
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=POT_SPI_CLK_PIN,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
    };

    spi_device_interface_config_t POTcfg={
        .clock_speed_hz=25*HZ_PER_MHZ,           //Clock out at 25 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=POT_CS,                   //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
    };

    ret=spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK){
        printf("failed to init bus\n");
    }

    ret=spi_bus_add_device(SPI2_HOST, &POTcfg, &pot);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK){
        printf("failed to init device\n");
    }

    return ESP_OK;
}

void pot_init(void)
{
    gpio_set_direction(POT_NSHUTDOWN, GPIO_MODE_OUTPUT);
    gpio_set_direction(POT_NSET_MID, GPIO_MODE_OUTPUT);
    gpio_set_level(POT_NSHUTDOWN, 1);   //Active low signal
    gpio_set_level(POT_NSET_MID, 1);    //Active low signal

    setPotResistance(0);
}

void pdu_input_init(void)
{
    gpio_set_direction(POW_AUX_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(POW_BMU_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(POW_BRAKE_LIGHT_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(POW_DCU_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(POW_LEFT_FAN_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(POW_LEFT_PUMP_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(POW_MC_LEFT_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(POW_MC_RIGHT_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(POW_RIGHT_FAN_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(POW_RIGHT_PUMP_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(POW_VCU_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(BATTERY_RAW_PIN, GPIO_MODE_INPUT);
}

void app_main(void)
{
    CAN_init();
    spi_init();
    pot_init();
    pdu_input_init();
    taskRegister();
}