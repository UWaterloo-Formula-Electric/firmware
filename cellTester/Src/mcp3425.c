#include "mcp3425.h"
#include "debug.h"

HAL_StatusTypeDef mcp3425_configure(I2C_HandleTypeDef *i2cmodule) {
    HAL_StatusTypeDef status;
    const uint8_t address_byte = DEVICE_CODE << 4; // R/W bit is 0
    const uint8_t config_byte = 0b10011000; // 16 bit resolution + default configs

    status = HAL_I2C_Master_Transmit(i2cmodule, address_byte, config_byte, 1, HAL_MAX_DELAY);
    if (status != HAL_OK) {
        ERROR_PRINT("Could not configure cell tester adc over i2c\n");
        return HAL_ERROR;
    }
    return HAL_OK;
}

// Also write the adc reading to provided address
HAL_StatusTypeDef mcp3425_read(I2C_HandleTypeDef *i2cmodule, uint16_t *data_out) {
    HAL_StatusTypeDef status;
    int16_t adc_raw;
    // According to datasheet
    const uint8_t address_byte = (DEVICE_CODE << 4) + 1;
    uint8_t rbuffer[2] = {0};

    status = HAL_I2C_Master_Transmit(i2cmodule, address_byte, rbuffer, sizeof(rbuffer), HAL_MAX_DELAY);
    if (status != HAL_OK) {
        ERROR_PRINT("Error reading thermistor ADC");
        return HAL_ERROR;
    } else {
        adc_raw = (rbuffer[0] << 8) + rbuffer[1];
        if(adc_raw > 32767) {
          adc_raw -= 65535;
        }
    }
    (*data_out) = adc_raw;
    return HAL_OK;
}