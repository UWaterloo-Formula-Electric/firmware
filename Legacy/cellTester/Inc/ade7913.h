#ifndef ADE7913_H
#define ADE7913_H

#include "stm32f0xx.h"
// THESE CONSTANTS WERE CALIBRATED WITH A WRONG raw -> VOLTAGE conversion
// Formula used: dataOut = scale * (raw + offset)
// Actual formula: dataOut = (scale * raw) + offset
// THIS WILL NOT WORK FOR FUTURE CALIBRATIONS as the underlying 
// formula was corrected in ade7913_common.c

//Voltage scale and offset convert to volts
//Scale found using the equation Vmax/2^23-1, 
//where Vmax is the max differential voltage between
//V1/V2 and Vm, the offset and scale were then manually calibrated
//V1 and V2 max is 500 mV
#define VOLTAGE_1_SCALE (0.00000884955752 / 1.0093885)
#define VOLTAGE_1_OFFSET (-411949 * VOLTAGE_1_SCALE)

#define VOLTAGE_2_SCALE (0.00000602006984F * 1.4486644)
#define VOLTAGE_2_OFFSET (-458819)

//Scale found with same process as voltage channels
//Vmax is the max differential voltage between 
//Ip and Im pins (31.25mv)
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

#endif /* ADE7913_H*/
