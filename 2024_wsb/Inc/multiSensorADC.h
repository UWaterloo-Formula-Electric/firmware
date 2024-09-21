#ifndef __MULTISENSORADC_H__
#define __MULTISENSORADC_H__

#include "stm32f4xx_hal.h"

#define NUM_ADC_CHANNELS 3  // BrakeIr, Sensor2, Sensor3

extern uint32_t multiSensorADCBuf[NUM_ADC_CHANNELS];
HAL_StatusTypeDef multi_sensor_adc_init(void);

#endif  // __MULTISENSORADC_H__