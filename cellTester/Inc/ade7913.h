#ifndef ADE7013_H
#define ADE7013_H

//addresses set in data sheet
#define ADDR_CURRENT (0x00)
#define ADDR_V1 (0x01)
#define ADDR_V2 (0x02)

#define ADDR_STATUS0 (0x9)
#define RESET_ON_BIT (0x0)

#define ADDR_CFG (0x8)
#define ADC_FREQ_2KHZ (0x20)
#define ADC_FREQ_1KHZ (0x30)
#define ADC_LOW_BW_ENABLE (0x80)

#define SPI_TIMEOUT 15

#define DISABLE_ADC_WARNINGS

// Voltage scale and offset convert to volts
#define VOLTAGE_1_SCALE  (-0.000052670388F)
#define VOLTAGE_1_OFFSET (20.451204825373F)

#define VOLTAGE_2_SCALE  (-0.000052273823F)
#define VOLTAGE_2_OFFSET (23.442233510687)

#define CURRENT_SCALE  (0.0000000055036067569447039)
#define CURRENT_OFFSET (-0.0023230F)
#define CURRENT_SHUNT_VAL_OHMS_HITL (0.000235F)
#define CURRENT_SHUNT_VAL_OHMS_CELL_TESTER (0.0001F)

#define MAX_CURRENT_ADC_VOLTAGE (0.03125F)

#define READ_EN 0x4
#define WRITE_EN 0xF8

HAL_StatusTypeDef hvadc_init();
HAL_StatusTypeDef adc_read(uint8_t addr, int32_t *dataOut);
HAL_StatusTypeDef adc_readbyte(uint8_t addr, uint8_t *dataOut);
HAL_StatusTypeDef adc_write(uint8_t addr, uint8_t data);
HAL_StatusTypeDef adc_read_current(float *dataOut);
HAL_StatusTypeDef adc_read_v1(float *dataOut);
HAL_StatusTypeDef adc_read_v2(float *dataOut);

#endif /* ADE7013_H*/
