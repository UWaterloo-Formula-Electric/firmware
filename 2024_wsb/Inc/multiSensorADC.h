#ifndef __MULTISENSORADC_H__
#define __MULTISENSORADC_H__

#include "stm32f4xx_hal.h"

#define NUM_ADC_CHANNELS 3 // BrakeIr, Sensor2, Sensor3

// Each element of this array will hold the value of the corresponding ADC channel
// TODO: Acc the order might not be as below, need to test
// buf[0] = BrakeIr, buf[1] = Sensor2, buf[2] = Sensor3, ...
extern uint32_t multiSensorADCBuf[NUM_ADC_CHANNELS];
HAL_StatusTypeDef multi_sensor_adc_init(void);

double getBrakeIrVoltage();
double getSensor2Voltage(); // TODO: Rename
double getSensor3Voltage(); // TODO: Rename

#endif // __MULTISENSORADC_H__