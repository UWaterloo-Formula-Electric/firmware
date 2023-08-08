#include "mcp3425.h"
#include "debug.h"
#include "task.h"
#include "i2c.h"

uint8_t rbuffer[3] = {0U, 0U, 0U};
const uint8_t address_byte = ((MCP3425_ADDR_BYTE) << 1); 

HAL_StatusTypeDef mcp3425_device_ready(I2C_HandleTypeDef *i2c_hdr) {
    HAL_StatusTypeDef status;

    status = HAL_I2C_IsDeviceReady(i2c_hdr, address_byte, I2C_INIT_TRIALS, I2C_INIT_TIMEOUT);
    if (status != HAL_OK) {
        ERROR_PRINT("ADC device not ready\n");
        return HAL_ERROR;
    } 
    return HAL_OK;
}

HAL_StatusTypeDef mcp3425_adc_configure(I2C_HandleTypeDef *i2c_hdr) {
    uint8_t tx_payload[4] = {MCP3425_CONFIG_BYTE, 0x00, 0x00, 0x00};

    // HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(i2c_hdr, address_byte, &config_byte, MCP3425_CONFIG_BYTE_SIZE, HAL_MAX_DELAY);
    HAL_I2C_Master_Transmit_DMA(i2c_hdr, address_byte, tx_payload, MCP3425_CONFIG_BYTE_SIZE+3);

    return HAL_OK;
}

// Also write the adc reading to provided address
HAL_StatusTypeDef mcp3425_adc_read(I2C_HandleTypeDef *i2c_hdr, int16_t *save_adc_output_val) {
    int16_t adc_raw;
    uint8_t config_byte;

    // HAL_StatusTypeDef status = HAL_I2C_Master_Receive(i2c_hdr, address_byte, rbuffer, sizeof(rbuffer), HAL_MAX_DELAY);
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive_DMA(i2c_hdr, address_byte, rbuffer, sizeof(rbuffer));
    if (status != HAL_OK) {
        ERROR_PRINT("Error reading thermistor ADC\n");
        return HAL_ERROR;
    } else {
        vTaskDelay(pdMS_TO_TICKS(1000));
        adc_raw = (rbuffer[0] << 8) | rbuffer[1];

        // Check RDY bit
        // 0 = Output register has been updated with the latest conversion data.
        config_byte = rbuffer[2];
        if (config_byte & MCP3425_CONFIG_RDY_BIT_MASK) {
            // Output register has not been updated
            DEBUG_PRINT("Output not updated by ADC\n");
        }
        else
        {
            DEBUG_PRINT("ADC output updated config: %u, adc: %d\r\n", config_byte, adc_raw);
        }
    }
    (*save_adc_output_val) = adc_raw * MCP3425_PGA_GAIN;
    return HAL_OK;
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance==hi2c1.Instance)
    {
        DEBUG_PRINT_ISR("I2C TX 1\r\n");
    }
    else if (hi2c->Instance==hi2c2.Instance)
    {
        DEBUG_PRINT_ISR("I2C TX 2\r\n");
    }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance==hi2c1.Instance)
    {
        DEBUG_PRINT_ISR("I2C RX 1\r\n");
    }
    else if (hi2c->Instance==hi2c2.Instance)
    {
        DEBUG_PRINT_ISR("I2C RX 2\r\n");
    }
}
