#include <stdio.h>
#include "dac.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "driver/spi_master.h"
#include "driver/dac_oneshot.h"

spi_device_handle_t throttle_A;
spi_device_handle_t throttle_B;
spi_device_handle_t brake_pos;
spi_device_handle_t steer_raw;

twai_message_t message_status = {
    .identifier = 0x8060F02,
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
    esp_err_t fault = 0;

    //above Vref, set to Vref, below 0 set to 0 
    if(voltage>VREF || voltage<0)
    {
        if(voltage>VREF)
        {
            printf("voltage too high\n");
            voltage=VREF;
        }
        else
        {
            voltage=0;
        }
    }

    uint8_t Vout=(voltage/1000)/STEPV8;
    
    //checking if channel already initialized
    if(channel1==false)
    {
        dac_oneshot_new_channel(&chan0_cfg, &chan0_handle);
        channel1 = true;
    }
    
    fault = dac_oneshot_output_voltage(chan0_handle,Vout);
    if(fault==ESP_OK)
    {
        message_status.data[0] = 1;
        twai_transmit(&message_status,portMAX_DELAY);
        printf("setting voltage to %fV in channel 1\n", voltage); 

        return ESP_OK;  
    }
    
    printf("Failed to set brake pres raw\r\n");
    return ESP_FAIL;
}

int set6551Voltage (float voltage, dacID id)
{
    //clamp values
    if(voltage>VREF || voltage<0)
    {
        if(voltage>VREF)
        {
            voltage=VREF;
        }
        else
        {
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
    spi_transaction_t trans = 
    {
        .tx_data [0] = BYTE0,
        .tx_data [1] = Byte1,
        .tx_data [2] = Byte2,
        .length = TXLENGTH,
        .flags = SPI_TRANS_USE_TXDATA,
    };

    esp_err_t fault = 0;

    if(id == brakePos_ID)
    {
        fault = spi_device_transmit(brake_pos, &trans);
        message_status.data[0] = 2;
    }
    else if(id == throttleA_ID)
    {
        fault = spi_device_transmit(throttle_A, &trans);
        message_status.data[0] = 8;
    }
    else if(id  == throttleB_ID)
    {
        fault = spi_device_transmit(throttle_B, &trans);
        message_status.data[0] = 16;
    }
    else if(id  == steerRaw_ID)
    {
        fault = spi_device_transmit(steer_raw, &trans);
        message_status.data[0] = 4;
    }

    if(fault != ESP_OK)
    {
        printf("Failed transmit data\n");
        return ESP_FAIL;
    }
    
    twai_transmit(&message_status,portMAX_DELAY);
    return ESP_OK;
}
