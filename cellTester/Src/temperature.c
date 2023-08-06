#include "temperature.h"
#include "debug.h"
#include <math.h>

// MCP3425
const uint8_t device_code = 0b1101;
const uint8_t address_byte = device_code << 4; // address bits = 000 programmed at factory

// Constants of Steinhart-Hart equation, calculated from samples from R-T table
#define A 0.0011144534f
#define B 0.0002364747f
#define C 0.0000000787784f

// Beta (K) 0-50 C (from the USP10982 datasheet)
#define BETA 3892
#define R1 10000
#define T1 298.15 // 25 Celsius

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
float NTC_V_to_R(float voltage) {
    // R1 == R2 == R3
    float R_t = R1 * (((VOLTAGE_IN*R1) - (voltage*(2*R1))) 
                        /((VOLTAGE_IN*R1) + (voltage*(2*R1))));
    return R_t;
}

float NTC_ADC_to_V(int ADC_ticks) {

}


// FreeRTOS task to periodically check thermistor temperature
void temperatureTask(void *pvParameters) {
    uint32_t temp_period = pdMS_TO_TICKS(TEMPERATURE_PERIOD_MS);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    int16_t adc_data;

    while (1) {

        vTaskDelayUntil(&xLastWakeTime, temp_period);
    }
}