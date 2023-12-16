#include <math.h>

#include "temperature.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "mcp3425.h"

I2C_HandleTypeDef *cell_i2c_hdr = &hi2c1;
I2C_HandleTypeDef *fuse_i2c_hdr = &hi2c2;

// Steinhart-Hart equation (https://en.wikipedia.org/wiki/Steinhart%E2%80%93Hart_equation)
double temp_steinhart_hart(double resistance) {
    double ln_R = log(resistance);
    double temp_K = 1 / (A + (B * ln_R) + (C * ln_R * ln_R * ln_R));
    double temp_C = KELVIN_TO_CELSIUS(temp_K);
    return temp_C;
} 

// Voltage to Resistance (Wheastone Bridge equation)
// https://www.ametherm.com/thermistor/ntc-thermistors-temperature-measurement-with-wheatstone-bridge
double ntc_V_to_R(double voltage) {
    // R1 == R2 == R3
    double R_t = R1 * (((VOLTAGE_IN*R1) - (voltage*(2*R1))) 
                        /((VOLTAGE_IN*R1) + (voltage*(2*R1))));
    //float R_t = (float)R1 / (1/(0.5-voltage/VOLTAGE_IN)-1);
    return R_t;
}

double adc_to_volts(int16_t adc_ticks) {
    double scaled_percentage = adc_ticks / ADC_MAX;
    double voltage = scaled_percentage * VOLTAGE_REF;
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

HAL_StatusTypeDef read_thermistor(I2C_HandleTypeDef *i2c_hdr, double *output_temperature) {
    if (mcp3425_adc_read(i2c_hdr) != HAL_OK) {
        // ERROR_PRINT("Failed to read from adc\n");
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

    double voltage = adc_to_volts(save_adc_output_val);
    double resistance = ntc_V_to_R(voltage);
    double temperature_celsius = temp_steinhart_hart(resistance);
    (*output_temperature) = temperature_celsius;
    //(*output_temperature) = resistance;

    return HAL_OK;
}

HAL_StatusTypeDef read_thermistor_CalMode(I2C_HandleTypeDef *i2c_hdr, double *output_temperature, double *output_voltage){
    if (mcp3425_adc_read(i2c_hdr) != HAL_OK) {
        // ERROR_PRINT("Failed to read from adc\n");
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

    double voltage = adc_to_volts(save_adc_output_val);
    double resistance = ntc_V_to_R(voltage);
    double temperature_celsius = temp_steinhart_hart(resistance);
    *output_temperature = temperature_celsius;
    *output_voltage = voltage * 1000;

    return HAL_OK;
}