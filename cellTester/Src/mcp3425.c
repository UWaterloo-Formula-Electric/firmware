#include "mcp3425.h"
#include "debug.h"
#include "i2c.h"


HAL_StatusTypeDef mcp3425_device_ready(I2C_HandleTypeDef *i2c_hdr) {
    HAL_StatusTypeDef status;
    const uint8_t address_byte = ADDR_CODE << 1;

    status = HAL_I2C_IsDeviceReady(i2c_hdr, address_byte, I2C_INIT_TRIALS, I2C_INIT_TIMEOUT);
    if (status != HAL_OK) {
        ERROR_PRINT("ADC device not ready\n");
        return HAL_ERROR;
    } 
    return HAL_OK;
}

HAL_StatusTypeDef mcp3425_adc_configure(I2C_HandleTypeDef *i2c_hdr) {
    const uint8_t address_byte = ADDR_CODE << 1; 
    uint8_t config_byte = CONFIG_BYTE;

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(i2c_hdr, address_byte, &config_byte, CONFIG_BYTE_SIZE, HAL_MAX_DELAY);
    if (status != HAL_OK) {
        ERROR_PRINT("Could not configure cell tester adc over i2c\n");
        return HAL_ERROR;
    }
    return HAL_OK;
}

// Also write the adc reading to provided address
HAL_StatusTypeDef mcp3425_adc_read(I2C_HandleTypeDef *i2c_hdr, int16_t *save_adc_output_val) {
    int16_t adc_raw;
    uint8_t config_byte;
    const uint8_t address_byte = (ADDR_CODE << 1) | MCP3425_READ_BIT;

    uint8_t rbuffer[3] = {0};

    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(i2c_hdr, address_byte, rbuffer, sizeof(rbuffer), HAL_MAX_DELAY);
    if (status != HAL_OK) {
        ERROR_PRINT("Error reading thermistor ADC\n");
        return HAL_ERROR;
    } else {
        adc_raw = (rbuffer[0] << 8) | rbuffer[1];

        // Check RDY bit
        // 0 = Output register has been updated with the latest conversion data.
        config_byte = rbuffer[2];
        if (config_byte & CONFIG_RDY_BIT_MASK) {
            // Output register has not been updated
            DEBUG_PRINT("Output not updated by ADC\n");
        }
    }
    (*save_adc_output_val) = adc_raw * MCP3425_PGA_GAIN;
    return HAL_OK;
}