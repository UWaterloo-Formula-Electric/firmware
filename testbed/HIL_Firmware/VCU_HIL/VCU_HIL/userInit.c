#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "driver/spi_master.h"
#include "userInit.h"
#include "dac.h"
#include "canReceive.h"
#include "processCAN.h"

spi_device_handle_t throttle_A;
spi_device_handle_t throttle_B;
spi_device_handle_t brake_pos;
spi_device_handle_t steer_raw;

void taskRegister (void)
{
    BaseType_t xReturned = pdPASS;
    TaskHandle_t can_rx = NULL;
    TaskHandle_t can_process = NULL;

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
        process_rx_task,
        "CAN_PROCESS_TASK",
        4000,
        ( void * ) 1,
        configMAX_PRIORITIES-1,
        &can_process
    );

    if(xReturned != pdPASS)
    {
        printf("Failed to register process_rx_task to RTOS");
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
        printf("TWAI driver installed\n");
    } 
    else 
    {
        printf("Failed to install TWAI driver\n");
        return ESP_FAIL;
    }

    if (twai_start() == ESP_OK) 
    {
        printf("TWAI driver started\n");
        return ESP_OK;
    } 
    else 
    {
        printf("Failed to start TWAI driver\n");
        return ESP_FAIL;
    }
}


int spi_init(void)
{
    printf("Initializing SPI bus\n");

    esp_err_t ret = ESP_OK;

    memset(&throttle_A, 0, sizeof(spi_device_handle_t));
    memset(&throttle_B, 0, sizeof(spi_device_handle_t));
    memset(&brake_pos, 0, sizeof(spi_device_handle_t));
    memset(&steer_raw, 0, sizeof(spi_device_handle_t));

    spi_bus_config_t buscfg={
        .mosi_io_num=SPI_MOSI_PIN,
        .sclk_io_num=SPI_CLK_PIN,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
    };

    spi_device_interface_config_t throttleACfg={
        .clock_speed_hz=20*HZ_PER_MHZ,           //Clock out at 20 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=THROTTLE_A_CS,            //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
    };

    spi_device_interface_config_t throttleBCfg={
        .clock_speed_hz=20*HZ_PER_MHZ,           //Clock out at 20 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=THROTTLE_B_CS,            //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
    };

    spi_device_interface_config_t brakePosCfg={
        .clock_speed_hz=20*HZ_PER_MHZ,           //Clock out at 20 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=BRAKE_POS_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
    };

    spi_device_interface_config_t steerRawCfg={
        .clock_speed_hz=20*HZ_PER_MHZ,           //Clock out at 10 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=STEER_RAW_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
    };

    ret=spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK){
        printf("failed to init SPI bus\r\n");
    }

    ret=spi_bus_add_device(SPI2_HOST, &throttleACfg, &throttle_A);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK){
        printf("failed to init throttle A DAC\r\n");
    }

    ret=spi_bus_add_device(SPI2_HOST, &throttleBCfg, &throttle_B);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK){
        printf("failed to init throttle B DAC\r\n");
    }

    ret=spi_bus_add_device(SPI2_HOST, &brakePosCfg, &brake_pos);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK){
        printf("failed to init brake position DAC\r\n");
    }

    ret=spi_bus_add_device(SPI2_HOST, &steerRawCfg, &steer_raw);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK){
        printf("failed to init steering raw DAC\r\n");
    }

    set6551Voltage(0,DacId_throttleA);
    set6551Voltage(0,DacId_throttleB);
    set6551Voltage(0,DacId_brakePos);
    set6551Voltage(0,DacId_steerRaws);

    return ESP_OK;
}

void app_main(void)
{
    spi_init();
    CAN_init();
    taskRegister();
}
