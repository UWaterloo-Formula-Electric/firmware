#include <stdio.h>
#include "dac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/twai.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/dac_oneshot.h"

spi_device_handle_t throttle_A;
spi_device_handle_t throttle_B;
spi_device_handle_t brake_pos;
spi_device_handle_t steer_raw;

twai_message_t message_status = {
    .identifier = 0x8060211,
    .extd = 1,
    .data_length_code = 1,
};

//convert VREF from mV to V
const double STEPV12 = (VREF/1000)/MAXSTEPS12;
const double STEPV8 = (VREF/1000)/MAXSTEPS8;

static bool channel1=false;

static dac_oneshot_handle_t chan0_handle;
static dac_oneshot_config_t chan0_cfg = {
    .chan_id = DAC_CHAN_0,
};

int setDacVoltage(float voltage)
{
    //above Vref, set to Vref, below 0 set to 0 
    if(voltage>VREF || voltage<0){
        if(voltage>VREF){
            printf("voltage too high\n");
            voltage=VREF;
        }
        else{
            voltage=0;
        }
    }

    uint8_t Vout=(voltage/1000)/STEPV8;
    
    //checking if channel already initialized
    if(channel1==false){
        dac_oneshot_new_channel(&chan0_cfg, &chan0_handle);
        channel1 = true;
    }
    
    ESP_ERROR_CHECK(dac_oneshot_output_voltage(chan0_handle,Vout));

    printf("setting voltage to %fV in channel 1\n", voltage);
    return ESP_OK;
}

int deleteChannel(uint32_t channel)
{
    if(channel != 2 && channel != 1){
        printf("Error! expecting channel 1 or 2!\n");

        return ESP_ERR_INVALID_ARG;
    }

    if(channel==1){
        if(channel1==true){

            dac_oneshot_del_channel(chan0_handle);
            channel1 = false;
            printf("channel %ld has been deleted\n", channel);
            return ESP_OK;
        }
        printf("ERROR! cannot delete channel that is not allocated\n");
        return ESP_ERR_INVALID_STATE;
    }

    printf("ERROR! cannot delete channel that is not allocated\n");
    
    return ESP_FAIL;
}

int set6551Voltage (float voltage, uint32_t id){

    //above 4095, set to 4095, below 0 set to 0 
    if(voltage>VREF || voltage<0){
        if(voltage>VREF){
            voltage=VREF;
        }
        else{
            voltage=0;
        }
    }
    
    //convert voltage from mV to V
    uint16_t Vout=(voltage/1000)/STEPV12;
    
    //D11 to D4 are placed into a second byte, while truncading the last 4 bits of data
    uint8_t Byte1 = Vout>>4;
    Vout &= 0b00001111;
    uint8_t Byte2 = Vout<<4;

    //data is less than 32 bits so must set each individual byte in tx_data and set SPI_TRANS_USE_TXDATA
    spi_transaction_t trans = {
        .tx_data [0] = BYTE0,
        .tx_data [1] = Byte1,
        .tx_data [2] = Byte2,
        .length = TXLENGTH,
        .flags = SPI_TRANS_USE_TXDATA,
    };

    esp_err_t fault = 0;

    if(id == brakePos_ID){
        fault = spi_device_transmit(brake_pos, &trans);
        message_status.data[0] = 2;
    }
    else if(id == throttleA_ID){
        fault = spi_device_transmit(throttle_A, &trans);
        message_status.data[0] = 8;
    }
    else if(id  == throttleB_ID){
        fault = spi_device_transmit(throttle_B, &trans);
        message_status.data[0] = 16;
    }
    else if(id  == steerRaw_ID){
        fault = spi_device_transmit(steer_raw, &trans);
        message_status.data[0] = 4;
    }

    if(fault != ESP_OK){
        printf("Failed transmit data\n");
        return ESP_FAIL;
    }
    
    twai_transmit(&message_status,portMAX_DELAY);
    return ESP_OK;
}

int spi_init(void){
    
    printf("Initializing SPI bus\n");

    esp_err_t ret;
    spi_bus_config_t buscfg={
        //.miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        //.max_transfer_sz=PARALLEL_LINES*320*2+8
    };

    spi_device_interface_config_t throttleAcfg={
        .clock_speed_hz=10*1000*1000,           //Clock out at 10 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=THROTTLE_A_CS,            //CS pin
        .queue_size=1,                          //We want to be able to queue 7 transactions at a time
    };

    spi_device_interface_config_t throttleBcfg={
        .clock_speed_hz=20*1000*1000,           //Clock out at 10 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=THROTTLE_B_CS,            //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
    };

    spi_device_interface_config_t brakePoscfg={
        .clock_speed_hz=20*1000*1000,           //Clock out at 10 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=BRAKE_POS_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
    };

    spi_device_interface_config_t steerRawcfg={
        .clock_speed_hz=20*1000*1000,           //Clock out at 10 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=STEER_RAW_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
    };

    ret=spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK){
        printf("failed to init bus\n");
    }

    ret=spi_bus_add_device(SPI2_HOST, &throttleAcfg, &throttle_A);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK){
        printf("failed to init device\n");
    }

    ret=spi_bus_add_device(SPI2_HOST, &throttleBcfg, &throttle_B);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK){
        printf("failed to init device\n");
    }

    ret=spi_bus_add_device(SPI2_HOST, &brakePoscfg, &brake_pos);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK){
        printf("failed to init device\n");
    }

    ret=spi_bus_add_device(SPI2_HOST, &steerRawcfg, &steer_raw);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK){
        printf("failed to init device\n");
    }

    set6551Voltage(0,throttleA_ID);
    set6551Voltage(0,throttleB_ID);
    set6551Voltage(0,brakePos_ID);
    set6551Voltage(0,steerRaw_ID);

    return ESP_OK;
}