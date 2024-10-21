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
#include "dac.h"

/***********************************
************ GLOBALS ***************
************************************/

spi_device_handle_t batt_pos;
spi_device_handle_t batt_neg;
spi_device_handle_t hv_neg_output;
spi_device_handle_t hv_pos_output;
spi_device_handle_t hv_shunt_pos;
spi_device_handle_t hv_shunt_neg;

// utilizes seperate isoSPI bus
spi_device_handle_t ams;

/***********************************
***** FUNCTION DEFINITIONS *********
************************************/
esp_err_t CAN_init (void)
{
    memset(&rx_msg, 0, sizeof(twai_message_t));
    memset(&can_msg, 0, sizeof(twai_message_t));
    memset(&bmu_hil_queue, 0, sizeof(QueueHandle_t)); //not 0 ing other queues to avoid repetition

    //TODO: these will probably have to be moved
    twai_handle_t twai_hil;
    twai_handle_t twai_chrgr;

    // Config Structures
    twai_general_config_t g_config = {
        .mode = TWAI_MODE_NORMAL, 
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

    // Install CAN HIL driver
    g_config.controller_id = CAN_HIL_ID,
    g_config.tx_io = CAN_HIL_TX, 
    g_config.rx_io = CAN_HIL_RX,
    if (twai_driver_install_v2(&g_config, &t_config, &f_config, &twai_hil) != ESP_OK)
    {
        printf("Failed to install TWAI driver\r\n");
        return ESP_FAIL;
    } 
    printf("TWAI driver installed\r\n");

    // Start CAN HIL driver
    if (twai_start_v2(&twai_hil) != ESP_OK) 
    {
        printf("Failed to start TWAI driver\r\n");
        return ESP_FAIL;
        
    } 
    printf("TWAI driver started\r\n");

    // Install CAN CHRGR driver
    g_config.controller_id = CAN_CHRGR_ID,
    g_config.tx_io = CAN_CHRGR_TX, 
    g_config.rx_io = CAN_CHRGR_RX,
    if (twai_driver_install_v2(&g_config, &t_config, &f_config, &twai_chrgr) != ESP_OK)
    {
        printf("Failed to install TWAI driver\r\n");
        return ESP_FAIL;
    } 
    printf("TWAI driver installed\r\n");

    // Start CAN CHRGR driver
    if (twai_start_v2(&twai_chrgr) != ESP_OK) 
    {
        printf("Failed to start TWAI driver\r\n");
        return ESP_FAIL;
        
    } 
    printf("TWAI driver started\r\n");
    return ESP_OK;
}

esp_err_t SPI_init(void)
{
    printf("Initializing SPI bus\r\n");

    esp_err_t ret = ESP_OK;

    memset(&batt_pos, 0, sizeof(spi_device_handle_t));
    memset(&batt_neg, 0, sizeof(spi_device_handle_t));
    memset(&hv_neg_output, 0, sizeof(spi_device_handle_t));
    memset(&hv_pos_output, 0, sizeof(spi_device_handle_t));
    memset(&hv_shunt_pos, 0, sizeof(spi_device_handle_t));
    memset(&hv_shunt_neg, 0, sizeof(spi_device_handle_t));

    memset(&ams, 0, sizeof(spi_device_handle_t));

    //Bus Configurations

    spi_bus_config_t bus_config = {
        .mosi_io_num = SPI_MOSI_PIN,
        .sclk_io_num = SPI_CLK_PIN,
        .quadwp_io_num = NOT_USED,
        .quadhd_io_num = NOT_USED,
    };

    spi_bus_config_t isoBus_config = {
        .miso_io_num = ISOSPI_MISO_PIN,
        .mosi_io_num = ISOSPI_MOSI_PIN,
        .sclk_io_num = ISOSPI_CS_PIN,
        .quadwp_io_num = NOT_USED,
        .quadhd_io_num = NOT_USED,
    };

    //Device Configurations

    spi_device_interface_config_t battPos_config = {
        .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
        .mode = 0,                                   //SPI mode 0
        .spics_io_num = BATT_POS_CS,                 //CS pin
        .queue_size = MAX_SPI_QUEUE_SIZE,     
    }

    spi_device_interface_config_t battNeg_config = {
        .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
        .mode = 0,                                   //SPI mode 0
        .spics_io_num = BATT_NEG_CS,                 //CS pin
        .queue_size = MAX_SPI_QUEUE_SIZE,     
    }

    spi_device_interface_config_t hvNegOut_config = {
        .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
        .mode = 0,                                   //SPI mode 0
        .spics_io_num = HV_NEG_OUT_CS,               //CS pin
        .queue_size = MAX_SPI_QUEUE_SIZE,     
    }

    spi_device_interface_config_t hvPosOut_config = {
        .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
        .mode = 0,                                   //SPI mode 0
        .spics_io_num = HV_POS_OUT_CS,               //CS pin
        .queue_size = MAX_SPI_QUEUE_SIZE,     
    }

    spi_device_interface_config_t hvShuntPos_config = {
        .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
        .mode = 0,                                   //SPI mode 0
        .spics_io_num = HV_SHUNT_POS_CS,             //CS pin
        .queue_size = MAX_SPI_QUEUE_SIZE,     
    }

    spi_device_interface_config_t hvShuntNeg_config = {
        .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
        .mode = 0,                                   //SPI mode 0
        .spics_io_num = HV_SHUNT_NEG_CS,             //CS pin
        .queue_size = MAX_SPI_QUEUE_SIZE,     
    }

    //TODO: clock speed for AMS isoSPI
    spi_device_interface_config_t ams_config = {
        .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
        .mode = 0,                                   //SPI mode 0
        .spics_io_num = AMS_CS,                      //CS pin
        .queue_size = MAX_SPI_QUEUE_SIZE,     
    }

    // Bus Initialization

    ret = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if(ret != ESP_OK){
        printf("failed to init SPI2 bus %d\r\n", ret);
        return ESP_FAIL;
    }

    ret = spi_bus_initialize(SPI3_HOST, &isoBus_config, SPI_DMA_CH_AUTO);
    if(ret != ESP_OK){
        printf("failed to init SPI3 bus %d\r\n", ret);
        return ESP_FAIL;
    }

    // Adding Devices

    ret = spi_bus_add_device(SPI2_HOST, &battPos_config, &batt_pos);
    if(ret != ESP_OK){
        printf("failed to add throttle batt_pos to bus %d\r\n", ret);
        return ESP_FAIL;
    }

    ret = spi_bus_add_device(SPI2_HOST, &battNeg_config, &batt_neg);
    if(ret != ESP_OK){
        printf("failed to add batt_neg to bus %d\r\n", ret);
        return ESP_FAIL;
    }

    ret = spi_bus_add_device(SPI2_HOST, &hvNegOut_config, &hv_neg_output);
    if(ret != ESP_OK){
        printf("failed to add hv_neg_output to bus %d\r\n", ret);
        return ESP_FAIL;
    }

        ret = spi_bus_add_device(SPI2_HOST, &hvPosOut_config, &hv_pos_output);
    if(ret != ESP_OK){
        printf("failed to add hv_pos_output to bus %d\r\n", ret);
        return ESP_FAIL;
    }

    ret = spi_bus_add_device(SPI2_HOST, &hvShuntPos_config, &hv_shunt_pos);
    if(ret != ESP_OK){
        printf("failed to add hv_shunt_pos to bus %d\r\n", ret);
        return ESP_FAIL;
    }

    ret = spi_bus_add_device(SPI2_HOST, &hvShuntNeg_config, &hv_shunt_neg);
    if(ret != ESP_OK){
        printf("failed to add hv_shunt_neg to bus %d\r\n", ret);
        return ESP_FAIL;
    }

    ret = spi_bus_add_device(SPI3_HOST, &ams_config, &hv_shunt_neg);
    if(ret != ESP_OK){
        printf("failed to add hv_shunt_neg to bus %d\r\n", ret);
        return ESP_FAIL;
    }

    // TODO: init devices (see VCU HIL)

    return ESP_OK;
}

esp_err_t GPIO_init (void)
{
    //SET DIRECTIONS
    //DCDC On
    gpio_set_direction(DCDC_ON_PIN, GPIO_MODE_INPUT);

    //Ampseal
    gpio_set_direction(CBRB_PRESS_PIN, GPIO_MODE_OUTPUT); 
    gpio_set_direction(IL_CLOSE_PIN, GPIO_MODE_OUTPUT); 
    gpio_set_direction(HVD_PIN, GPIO_MODE_OUTPUT); 
    gpio_set_direction(TSMS_FAULT_PIN, GPIO_MODE_OUTPUT); 

    //Contactor Control
    gpio_set_direction(CONT_NEG_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CONT_POS_PIN, GPIO_MODE_INPUT);

    //Reset Buttons
    gpio_set_direction(AMS_RESET_PRESS_PIN, GPIO_MODE_OUTPUT); 
    gpio_set_direction(IMD_RESET_PRESS_PIN, GPIO_MODE_OUTPUT); 
    gpio_set_direction(BPSD_RESET_PRESS_PIN, GPIO_MODE_OUTPUT); 

    //IMD
    gpio_set_direction(IMD_FAULT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(IMD_STATUS_PIN, GPIO_MODE_OUTPUT);

    //Fan
    gpio_set_direction(FAN_PWM_PIN, GPIO_MODE_INPUT); //Will be measuring duty cycle (BMU sends 0-100% at 88Hz as defined by datasheet)
    gpio_set_direction(FAN_TACH_PIN, GPIO_MODE_OUTPUT); 
    
    //INITIALIZE OUTPUTS
    gpio_set_level(CBRB_PRESS_PIN,0);
    gpio_set_level(IL_CLOSE_PIN,0);
    gpio_set_level(HVD_PIN,0);
    gpio_set_level(TSMS_FAULT_PIN,0);

    gpio_set_level(AMS_RESET_PRESS_PIN,1);
    gpio_set_level(IMD_RESET_PRESS_PIN,1);
    gpio_set_level(BPSD_RESET_PRESS_PIN,1);

    gpio_set_level(IMD_FAULT_PIN,0); //Set to HIGH to signify fault. Using open drain, BMU recognizes 0V as a fault.
    gpio_set_level(IMD_STATUS_PIN,0); //TODO: PWM Signal 9MHz to 55MHz with duty cycle of 0-100%

    gpio_set_level(FAN_TACH_PIN,0); //TODO: Send PWM signal to BMU (0-150Hz. Freq = 1/((60/rpm)/2) with max rpm 4500 as per datasheet)

    return ESP_OK;
}

void app_main(void)
{
    CAN_init();
    SPI_init();
    GPIO_init();
}