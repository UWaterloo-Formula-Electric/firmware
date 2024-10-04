#ifndef ADE7913_H_
#define ADE7913_H_

#include <stdint.h>
#include "boardTypes.h"
#include "stm32f7xx.h"

#define ISO_ADC_SPI_HANDLE HV_ADC_SPI_HANDLE
#define ISO_ADC_CS_GPIO_Port HV_ADC_SPI_NSS_GPIO_Port

#define ISO_ADC_CS_Pin HV_ADC_SPI_NSS_Pin

// Voltage scale and offset convert to volts
#define VOLTAGE_1_SCALE  (0.000106316059927)
#define VOLTAGE_1_OFFSET (-49.9557902385)

#define VOLTAGE_2_SCALE  (0.000105423274447)
#define VOLTAGE_2_OFFSET (-55.5932673894)

#define CURRENT_SCALE  (-0.0000247518625777)
#define CURRENT_OFFSET (11.6704537017)

#define SELF_CALIBRATION_NUM_SAMPLES (50)
#define ADE7913_FS_ANALOG_MV  (0.03125f)      // ADC is accurate up to 31.25 mV 
#define ADE7913_FS_DIGITAL  (5320000.0f)      // 5,320,000 nominally at 31.25 mV

#define CURRENT_SHUNT_VAL_OHMS (0.0001f)
#define NEW_CURRENT_SCALE  (0.9448234107)    // inverse of 1.058398838

HAL_StatusTypeDef hvadc_init();
HAL_StatusTypeDef adc_read(uint8_t addr, int32_t *dataOut);
HAL_StatusTypeDef adc_readbyte(uint8_t addr, uint8_t *dataOut);
HAL_StatusTypeDef adc_write(uint8_t addr, uint8_t data);
HAL_StatusTypeDef adc_read_current(float *dataOut);
HAL_StatusTypeDef adc_read_v1(float *dataOut);
HAL_StatusTypeDef adc_read_v2(float *dataOut);

#endif /* ADE7913_H_*/
