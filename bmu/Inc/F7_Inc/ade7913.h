#ifndef ADE7013_H_
#define ADE7013_H_

#include <stdint.h>
#include "boardTypes.h"

#if IS_BOARD_F7
    #include "stm32f7xx.h"
#elif IS_BOARD_F0
    #include "stm32f0xx.h"
#else
    #error "Board Type Not Recognized"
#endif // BoardType

#define ISO_ADC_SPI_HANDLE HV_ADC_SPI_HANDLE
#define ISO_ADC_CS_GPIO_Port HV_ADC_SPI_NSS_GPIO_Port

#define ISO_ADC_CS_Pin HV_ADC_SPI_NSS_Pin

// Voltage scale and offset convert to volts
#define VOLTAGE_1_SCALE  (-0.000052670388F)
#define VOLTAGE_1_OFFSET (20.451204825373F)

#define VOLTAGE_2_SCALE  (-0.000052273823F)
#define VOLTAGE_2_OFFSET (23.442233510687)

#define CURRENT_SCALE  (0.0000000055036067569447039)
#define CURRENT_OFFSET (-0.0023230F)
#define CURRENT_SHUNT_VAL_OHMS (0.0001F)

HAL_StatusTypeDef hvadc_init();
HAL_StatusTypeDef adc_read(uint8_t addr, int32_t *dataOut);
HAL_StatusTypeDef adc_readbyte(uint8_t addr, uint8_t *dataOut);
HAL_StatusTypeDef adc_write(uint8_t addr, uint8_t data);
HAL_StatusTypeDef adc_read_current(float *dataOut);
HAL_StatusTypeDef adc_read_v1(float *dataOut);
HAL_StatusTypeDef adc_read_v2(float *dataOut);

#endif /* ADE7013_H_*/
