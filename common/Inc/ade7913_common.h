#ifndef ADE7013_COMMON_H
#define ADE7013_COMMON_H

#include "ade7913.h"

HAL_StatusTypeDef hvadc_init();
HAL_StatusTypeDef adc_read(uint8_t addr, int32_t *dataOut);
HAL_StatusTypeDef adc_readbyte(uint8_t addr, uint8_t *dataOut);
HAL_StatusTypeDef adc_write(uint8_t addr, uint8_t data);
HAL_StatusTypeDef adc_read_current(float *dataOut);
HAL_StatusTypeDef adc_read_v1(float *dataOut);
HAL_StatusTypeDef adc_read_v2(float *dataOut);
HAL_StatusTypeDef adc_read_v(float *dataOut);

#endif /* ADE7013_COMMON_H*/
