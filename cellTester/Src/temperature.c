#include "temperature.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "mcp3425.h"
#include <math.h>

I2C_HandleTypeDef *cell_i2c_hdr = &hi2c1;
I2C_HandleTypeDef *fuse_i2c_hdr = &hi2c2;

// Steinhart-Hart equation (https://en.wikipedia.org/wiki/Steinhart%E2%80%93Hart_equation)
float temp_steinhart_hart(float resistance) {
    float ln_R = log(resistance);
    float temp_K = 1 / (A + (B * ln_R) + (C * ln_R * ln_R * ln_R));
    float temp_C = KELVIN_TO_CELSIUS(temp_K);
    return temp_C;
}

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
    HAL_StatusTypeDef config_status = mcp3425_adc_configure(i2c_hdr);

    if (config_status != HAL_OK) {
        DEBUG_PRINT("config failed\r\n");
        return HAL_ERROR;
    }
    return HAL_OK;
}

float read_thermistor(I2C_HandleTypeDef *i2c_hdr, float *output_temperature) {
    if (mcp3425_adc_read(i2c_hdr) != HAL_OK) {
        ERROR_PRINT("Failed to read from adc\n");
        return HAL_ERROR;
    }

    int save_adc_output_val = 0;
    if (i2c_hdr == cell_i2c_hdr)
    {
        save_adc_output_val = adc_1_output_val;
    }
    else if (i2c_hdr == fuse_i2c_hdr)
    {
        save_adc_output_val = adc_2_output_val;
    }

    float voltage = adc_to_volts(save_adc_output_val);
    float resistance = ntc_V_to_R(voltage);
    float temperature_celsius = temp_steinhart_hart(resistance);
    (*output_temperature) = temperature_celsius;
    return HAL_OK;
}

// FreeRTOS task to periodically check thermistor temperatures
// todo - maybe: break the two thermistors into two tasks?
// uart transmit the data out?
void temperatureTask(void *pvParameters) {
    const uint32_t temp_period = pdMS_TO_TICKS(TEMPERATURE_PERIOD_MS);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    float cell_temp_result = 0.0f;
    float fuse_temp_result = 0.0f;

    // Wait for boot
    vTaskDelay(pdMS_TO_TICKS(100));

    // Configure ADCs    
    thermistor_adc_init(cell_i2c_hdr);
    thermistor_adc_init(fuse_i2c_hdr);

    vTaskDelay(100);

    while (1) {
        if (read_thermistor(cell_i2c_hdr, &cell_temp_result) != HAL_OK) {
            DEBUG_PRINT("failed to read cell temp\n");
        }

        if (read_thermistor(fuse_i2c_hdr, &fuse_temp_result) != HAL_OK) {
            DEBUG_PRINT("failed to read fuse temp\n");
        }
        
        vTaskDelayUntil(&xLastWakeTime, temp_period);
    }
}