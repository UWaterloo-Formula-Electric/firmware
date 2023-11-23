#ifndef CELL_TESTER_TEMP_H
#define CELL_TESTER_TEMP_H

#include "debug.h"
#include "i2c.h"

#define VOLTAGE_REF 2.048f
#define K_TO_C_CONVERSION 273.15f
#define VOLTAGE_IN 3.3f
#define ADC_MAX 32767.0f
#define KELVIN_TO_CELSIUS(K) ((K)-(K_TO_C_CONVERSION))

#define TEMPERATURE_PERIOD_MS 100

// Constants of Steinhart-Hart equation, calculated from samples from R-T table
#define A 0.0011144534f
#define B 0.0002364747f
#define C 0.0000000787784f

// Beta (K) 0-50 C (from the USP10982 datasheet)
#define BETA 3892
#define R1 10000
#define T1 298.15 // 25 Celsius

float temp_steinhart_hart(float resistance);
float ntc_V_to_R(float voltage);
float adc_to_volts(int16_t adc_ticks);
HAL_StatusTypeDef thermistor_adc_init(I2C_HandleTypeDef *i2c_hdr);
HAL_StatusTypeDef read_thermistor(I2C_HandleTypeDef *i2c_hdr, float *temp_output_val);

extern I2C_HandleTypeDef *cell_i2c_hdr;
extern I2C_HandleTypeDef *fuse_i2c_hdr;
#endif // CELL_TESTER_TEMP_H
