#ifndef CELL_TESTER_TEMP
#define CELL_TESTER_TEMP

#define VOLTAGE_REF 2.048f
#define K_TO_C_CONVERSION 273.15f
#define VOLTAGE_IN 3.3f
#define ADC_MAX 32767

#define TEMPERATURE_PERIOD_MS 500

float temp_steinhart_hart(float resistance);
float temp_beta(float resistance);
float ntc_V_to_R(float voltage);
float adc_to_volts(int16_t adc_ticks);
float read_thermistor(I2C_HandleTypeDef *i2cmodule);

#endif
