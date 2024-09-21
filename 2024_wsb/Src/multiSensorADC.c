#include "multiSensorADC.h"

#include "bsp.h"
#include "stm32f4xx_hal.h"

uint32_t multiSensorADCBuf[NUM_ADC_CHANNELS];

HAL_StatusTypeDef multi_sensor_adc_init(void) {
    return HAL_ADC_Start_DMA(&MULTISENSOR_ADC_HANDLE, multiSensorADCBuf, NUM_ADC_CHANNELS);
}

double adcToVoltage(uint32_t adcValue) {
    return (double)adcValue * (3.3 / 4095);
}

double getBrakeIrVoltage() {
    return adcToVoltage(multiSensorADCBuf[0]);
}

double getSensor2Voltage() {
    return adcToVoltage(multiSensorADCBuf[1]);
}

double getSensor3Voltage() {
    return adcToVoltage(multiSensorADCBuf[2]);
}