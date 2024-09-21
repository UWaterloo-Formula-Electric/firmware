#include "multiSensorADC.h"

#include "stm32f4xx_hal.h"

uint32_t multiSensorADCBuf[NUM_ADC_CHANNELS];

HAL_StatusTypeDef multi_sensor_adc_init(void) {
    // TODO: implement dma
    return HAL_OK;
}