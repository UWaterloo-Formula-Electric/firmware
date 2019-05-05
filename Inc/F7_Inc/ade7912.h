#ifndef ADE7012_H_
#define ADE7012_H_

HAL_StatusTypeDef hvadc_init();
int32_t adc_read(uint8_t addr);
HAL_StatusTypeDef adc_readbyte(uint8_t addr, uint8_t *dataOut);
HAL_StatusTypeDef adc_write(uint8_t addr, uint8_t data);
uint32_t adc_read_current(void);
uint32_t adc_read_v1(void);
uint32_t adc_read_v2(void);

#endif /* ADE7012_H_*/
