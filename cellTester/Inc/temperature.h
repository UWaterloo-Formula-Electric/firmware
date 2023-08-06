#ifndef CELL_TESTER_TEMP
#define CELL_TESTER_TEMP

#define VOLTAGE_REF 3.3f
#define K_TO_C_CONVERSION 273.15f
#define VOLTAGE_IN 3.3f

#define TEMPERATURE_PERIOD_MS 500

float cell_test_resistance_to_temp(float resistance);
float cell_test_voltage_to_resistance(float voltage);
float cell_test_ADC_to_voltage(int ADC_ticks);

#endif
