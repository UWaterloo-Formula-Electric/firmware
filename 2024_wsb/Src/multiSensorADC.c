#include "multiSensorADC.h"

#include "bsp.h"
#include "stm32f4xx_hal.h"

multiSensorADC_t multiSensorADC;

HAL_StatusTypeDef multi_sensor_adc_init(void) {
    return HAL_ADC_Start_DMA(&MULTISENSOR_ADC_HANDLE, multiSensorADC.multiSensorADCBuf, NUM_ADC_CHANNELS);
}
