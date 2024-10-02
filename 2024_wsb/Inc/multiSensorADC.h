#ifndef __MULTISENSORADC_H__
#define __MULTISENSORADC_H__

#include "stm32f4xx_hal.h"

#define NUM_ADC_CHANNELS 3 // Sensor1, Sensor2, Sensor3

#define ADC_TO_VOLTAGE(adcValue) ((float)adcValue * (3.3f / 4095f))

#define get_sensor1_V() ADC_TO_VOLTAGE(multiSensorADC.sensors.sensor1)
#define get_sensor2_V() ADC_TO_VOLTAGE(multiSensorADC.sensors.sensor2)
#define get_sensor3_V() ADC_TO_VOLTAGE(multiSensorADC.sensors.sensor3)

// Each element of this array will hold the value of the corresponding ADC channel
// TODO: Acc the order might not be as below, need to test
// buf[0] = Sensor1, buf[1] = Sensor2, buf[2] = Sensor3, ...
typedef union {
    uint32_t multiSensorADCBuf[NUM_ADC_CHANNELS];
    struct {
        uint32_t sensor1;
        uint32_t sensor2;
        uint32_t sensor3;
    } sensors;
} multiSensorADC_t;

extern multiSensorADC_t multiSensorADC;
HAL_StatusTypeDef multi_sensor_adc_init(void);



#endif // __MULTISENSORADC_H__