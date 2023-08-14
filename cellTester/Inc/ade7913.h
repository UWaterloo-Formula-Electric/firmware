#ifndef ADE7013_H
#define ADE7013_H

#include "stm32f0xx.h"

// Voltage scale and offset convert to volts
//Scale found by manually calculating using the equation
#define VOLTAGE_1_SCALE ((0.00000884955752)/1.0093885)
#define VOLTAGE_1_OFFSET (-411949 * VOLTAGE_1_SCALE)

#define VOLTAGE_2_SCALE (0.00000602006984F)*1.4486644
#define VOLTAGE_2_OFFSET (-458819)

#define CURRENT_SCALE  (0.0000000055036067569447039)
#define CURRENT_OFFSET (-0.00229137)

#define CURRENT_SHUNT_VAL_OHMS (0.0001F)

HAL_StatusTypeDef hvadc_init();
HAL_StatusTypeDef adc_read(uint8_t addr, int32_t *dataOut);
HAL_StatusTypeDef adc_readbyte(uint8_t addr, uint8_t *dataOut);
HAL_StatusTypeDef adc_write(uint8_t addr, uint8_t data);
HAL_StatusTypeDef adc_read_current(float *dataOut);
HAL_StatusTypeDef adc_read_v1(float *dataOut);
HAL_StatusTypeDef adc_read_v2(float *dataOut);

#endif /* ADE7013_H*/
