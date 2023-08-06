#include "temperature.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "mcp3425.h"
#include <math.h>

// May need to reverse
I2C_HandleTypeDef *cell_i2c_hdr = &hi2c1;
I2C_HandleTypeDef *fuse_i2c_hdr = &hi2c2;

// Steinhart-Hart equation (https://en.wikipedia.org/wiki/Steinhart%E2%80%93Hart_equation)
float temp_steinhart_hart(float resistance) {
    float ln_R = log(resistance);
    float temp_K = 1 / (A + (B * ln_R) + (C * ln_R * ln_R * ln_R));
    float temp_C = temp_K - K_TO_C_CONVERSION;
    return temp_C;
}

// Simplified form of the Steinhart-Hart
float temp_beta(float resistance) {
    float ln_R1_over_R2 = log(R1/resistance);
    float temp_K = 1 / (((-1 * ln_R1_over_R2)/BETA) + (1/T1));
    float temp_C = temp_K - K_TO_C_CONVERSION;
    return temp_C;
}

// Potential third (and more performant) method of deriving temperature from resistance
// float temp_lookuptable(float resistance) {}

// Voltage to Resistance (Wheastone Bridge equation)
// https://www.ametherm.com/thermistor/ntc-thermistors-temperature-measurement-with-wheatstone-bridge
float ntc_V_to_R(float voltage) {
    // R1 == R2 == R3
    float R_t = R1 * (((VOLTAGE_IN*R1) - (voltage*(2*R1))) 
                        /((VOLTAGE_IN*R1) + (voltage*(2*R1))));
    return R_t;
}

float adc_to_volts(int16_t adc_ticks) {
    float scaled_percentage = adc_ticks / ADC_MAX;
    float voltage = scaled_percentage * VOLTAGE_REF;
    return voltage;
}

HAL_StatusTypeDef thermistor_adc_init(I2C_HandleTypeDef *i2c_hdr) {
    HAL_StatusTypeDef ready_status = mcp3425_device_ready(i2c_hdr);
    HAL_StatusTypeDef config_status = mcp3425_device_configure(i2c_hdr);

    if (ready_status != HAL_OK || config_status != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}

float read_thermistor(I2C_HandleTypeDef *i2c_hdr) {
    int16_t adc_result;
    if (mcp3425_read(i2c_hdr, &adc_result) != HAL_OK) {
        ERROR_PRINT("Failed to read cell temp from adc");
        return -1;
    }
    float voltage = adc_to_volts(adc_result);
    float resistance = ntc_V_to_R(voltage);
    float temperature_celsius = temp_steinhart_hart(resistance);
    return temperature_celsius;
}

// FreeRTOS task to periodically check thermistor temperatures
// todo - maybe: break the two thermistors into two tasks?
// uart transmit the data out?
void temperatureTask(void *pvParameters) {
    uint32_t temp_period = pdMS_TO_TICKS(TEMPERATURE_PERIOD_MS);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // Configure ADCs
    if (thermistor_adc_init(cell_i2c_hdr) != HAL_OK) {
        ERROR_PRINT("Failed to init cell temp adc\n");
    }
    if (thermistor_adc_init(fuse_i2c_hdr) != HAL_OK) {
        ERROR_PRINT("Failed to init fuse temp adc\n");
    }

    while (1) {
        float cell_temp = read_thermistor(cell_i2c_hdr);
        float fuse_temp = read_thermistor(fuse_i2c_hdr);

        DEBUG_PRINT("cell temp: %f\n", cell_temp);
        DEBUG_PRINT("fuse temp: %f\n", fuse_temp);

        // todo - log and/or transmit the data via UART?
        
        vTaskDelayUntil(&xLastWakeTime, temp_period);
    }
}