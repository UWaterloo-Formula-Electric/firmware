#include <stdio.h>
#include "dac.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "driver/spi_master.h"
#include "driver/dac_oneshot.h"

//retain 4 least significant bits
#define V_OUT_MASK 0x0F;

twai_message_t message_status = {
    .identifier = MESSAGE_STATUS,
    .extd = 1,
    .data_length_code = 1,
};

//convert V_REF from mV to V
const double STEPV12 = (V_REF/1000)/MAX_STEPS_12;
const double STEPV8 = (V_REF/1000)/MAX_STEPS_8;

static bool channel1=false;

static dac_oneshot_handle_t chan0_handle;
static dac_oneshot_config_t chan0_cfg = {
    .chan_id = DAC_CHAN_0,
};

int setDacVoltage(float voltage)
{
    esp_err_t fault = ESP_OK;

    //above Vref, set to Vref, below 0 set to 0 
    if(voltage > V_REF || voltage < 0.0f)
    {
        if(voltage > V_REF)
        {
            printf("voltage too high\n");
            voltage = V_REF;
        }
        else
        {
            voltage = 0.0f;
        }
    }

    uint8_t Vout = (voltage/1000)/STEPV8;
    
    //checking if channel already initialized
    if(channel1 == false)
    {
        dac_oneshot_new_channel(&chan0_cfg, &chan0_handle);
        channel1 = true;
    }
    
    fault = dac_oneshot_output_voltage(chan0_handle, Vout);
    if(fault == ESP_OK)
    {
        message_status.data[0] = 1;
        twai_transmit(&message_status, portMAX_DELAY);
        printf("setting voltage to %fV in channel 1\n", voltage); 

        return ESP_OK;  
    }
    
    printf("Failed to set brake pres raw\r\n");
    return ESP_FAIL;
}

int set6551Voltage (float voltage, DacId_E id)
{
    //clamp values
    if(voltage > V_REF || voltage < 0.0f)
    {
        if(voltage > V_REF)
        {
            voltage = V_REF;
        }
        else
        {
            voltage = 0.0f;
        }
    }
    
    //convert voltage from mV to V
    uint16_t Vout = (voltage/1000)/STEPV12;
    
    //D11 to D4 are placed into a second byte, while truncading the last 4 bits of data
    uint8_t Byte_1 = Vout >> 4;
    Vout &= V_OUT_MASK;
    uint8_t Byte_2 = Vout << 4;

    //data is less than 32 bits so must set each individual byte in tx_data and set SPI_TRANS_USE_TXDATA
    spi_transaction_t trans = 
    {
        .tx_data [0] = BYTE_0,
        .tx_data [1] = Byte_1,
        .tx_data [2] = Byte_2,
        .length = TX_LENGTH,
        .flags = SPI_TRANS_USE_TXDATA,
    };

    esp_err_t fault = ESP_OK;

    switch (id)
    {
        case DacId_brakePos:
            fault = spi_device_transmit(brake_pos, &trans);
            message_status.data[0] = BRAKE_POS_IS_SET;
            break;
        case DacId_steerRaws:
            fault = spi_device_transmit(steer_raw, &trans);
            message_status.data[0] = STEER_RAW_IS_SET;
            break;
        case DacId_throttleA:
            fault = spi_device_transmit(throttle_A, &trans);
            message_status.data[0] = THROTTLE_A_IS_SET;
            break;
        case DacId_throttleB:
            fault = spi_device_transmit(throttle_B, &trans);
            message_status.data[0] = THROTTLE_B_IS_SET;
            break;
        default:
            break;
    }

    if(fault != ESP_OK)
    {
        printf("Failed transmit data\n");
        return ESP_FAIL;
    }
    
    twai_transmit(&message_status, portMAX_DELAY);
    return ESP_OK;
}
