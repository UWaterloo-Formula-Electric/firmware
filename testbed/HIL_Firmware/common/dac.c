#include <stdio.h>
#include "dac.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "driver/spi_master.h"
#include "driver/dac_oneshot.h"

//retain 4 least significant bits
#define V_OUT_MASK 0x0F

twai_message_t message_status = {
    .identifier = MESSAGE_STATUS_CAN_ID,
    .extd = EXTENDED_MSG,
    .data_length_code = CAN_MSG_DATA_SIZE,
};

//convert V_REF_MV from mV to V
const double DAC_CONVERSION_12_BIT = (V_REF_MV / 1000) / MAX_12_BIT_VAL;
const double DAC_CONVERSION_8_BIT = (V_REF_MV / 1000) / MAX_8_BIT_VAL;

dac_oneshot_handle_t brake_pres_raw;

esp_err_t setDacVoltage(float voltage)
{
    esp_err_t fault = ESP_OK;

    //above Vref, set to Vref, below 0 set to 0 
    if(voltage > V_REF_MV)
    {
        printf("voltage too high\r\n");
        voltage = V_REF_MV;
    }
    else if(voltage < 0)
    {
        printf("volage must be greater than 0\r\n");
        voltage = 0.0f;
    }

    uint8_t output_voltage = (voltage / 1000) / DAC_CONVERSION_8_BIT;
    
    fault = dac_oneshot_output_voltage(brake_pres_raw, output_voltage);
    if(fault != ESP_OK)
    {
        printf("Failed to set brake pres raw %d\r\n", fault);
        return ESP_FAIL; 
    }
    
    message_status.data[0] = BRAKE_PRES_RAW_DAC_SET;
    twai_transmit(&message_status, portMAX_DELAY);
    printf("setting voltage to %fmV in channel 1\n", voltage);
    return ESP_OK;
}

esp_err_t set6551Voltage (float voltage, DacId_E id)
{
    //above Vref, set to Vref, below 0 set to 0 
    if(voltage > V_REF_MV)
    {
        printf("voltage too high\r\n");
        voltage = V_REF_MV;
    }
    else if(voltage < 0.0f)
    {
        printf("volage must be greater than 0\r\n");
        voltage = 0.0f;
    }
    
    //convert voltage from mV to V
    const uint16_t output_voltage = (voltage / 1000) / DAC_CONVERSION_12_BIT;
    
    //D11 to D4 are placed into a second byte, while truncading the last 4 bits of data
    uint8_t byte_1 = output_voltage >> 4;
    uint8_t byte_2 = (output_voltage & V_OUT_MASK) << 4;

    //data is less than 32 bits so must set each individual byte in tx_data and set SPI_TRANS_USE_TXDATA
    spi_transaction_t trans = 
    {
        .tx_data [0] = BYTE_0,
        .tx_data [1] = byte_1,
        .tx_data [2] = byte_2,
        .length = MAX_SPI_QUEUE_LENGTH,
        .flags = SPI_TRANS_USE_TXDATA,
    };

    esp_err_t fault = ESP_OK;

    switch (id)
    {
        case DacId_BrakePos:
            fault = spi_device_transmit(brake_pos, &trans);
            message_status.data[0] = BRAKE_POS_DAC_SET;
            break;
        case DacId_SteerRaw:
            fault = spi_device_transmit(steer_raw, &trans);
            message_status.data[0] = STEER_RAW_DAC_SET;
            break;
        case DacId_ThrottleA:
            fault = spi_device_transmit(throttle_A, &trans);
            message_status.data[0] = THROTTLE_A_DAC_SET;
            break;
        case DacId_ThrottleB:
            fault = spi_device_transmit(throttle_B, &trans);
            message_status.data[0] = THROTTLE_B_DAC_SET;
            break;
        default:
            printf("unknown dac ID %d", id);
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
