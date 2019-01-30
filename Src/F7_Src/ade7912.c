#include "bsp.h"
#include "debug.h"

#define ADDR_CURRENT (0x00)
#define ADDR_V1 (0x01)
#define ADDR_V2 (0x02)

#define SPI_TIMEOUT 15

HAL_StatusTypeDef adc_spi_tx(uint8_t * tdata, unsigned int len) {
    HAL_GPIO_WritePin(HV_ADC_SPI_NSS_GPIO_Port, HV_ADC_SPI_NSS_Pin , GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&HV_ADC_SPI_HANDLE, tdata, len, SPI_TIMEOUT);
    HAL_GPIO_WritePin(HV_ADC_SPI_NSS_GPIO_Port, HV_ADC_SPI_NSS_Pin , GPIO_PIN_SET);
    return status;
}

HAL_StatusTypeDef adc_spi_tx_rx(uint8_t * tdata, uint8_t * rbuffer, unsigned int len) {
    HAL_GPIO_WritePin(HV_ADC_SPI_NSS_GPIO_Port, HV_ADC_SPI_NSS_Pin , GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&HV_ADC_SPI_HANDLE, tdata, rbuffer, len, SPI_TIMEOUT);
    HAL_GPIO_WritePin(HV_ADC_SPI_NSS_GPIO_Port, HV_ADC_SPI_NSS_Pin , GPIO_PIN_SET);
    return status;
}

// also return data
uint8_t adc_readbyte(uint8_t addr) {
  HAL_StatusTypeDef status;
  uint8_t rbuffer[2] = {0};
  uint8_t tbuffer[2] = {0};

  //modifying address to datasheet format
  addr <<= 3; 
  addr |= 0x4;
  tbuffer[0] = addr;

  status = adc_spi_tx_rx(tbuffer, rbuffer, 2);
  DEBUG_PRINT("%x %x\n", rbuffer[0], rbuffer[1]);
  if (status != HAL_OK) {
    ERROR_PRINT("error reading :(\n");
    return 0;
  }
  return rbuffer[1];
}

//return the data from the address
int32_t adc_read(uint8_t addr){
  HAL_StatusTypeDef status;
  uint8_t rbuffer[4] = {0};
  uint8_t tbuffer[4] = {0};
  int32_t data = 0x0;

  //modifying address to datasheet format
  addr <<= 3; 
  addr |= 0x4;
  tbuffer[0] = addr;

  status = adc_spi_tx_rx(tbuffer, (uint8_t *)rbuffer, 4);

  if (status != HAL_OK){
    ERROR_PRINT("---Error reading from ADC! Status code: %d\n", status);
    return 0;
  }

  //put data into an u32. Skipping rbuffer[0] bc it contains high impedance GARBAGE


  // shift to upper three bytes, then right-shift one byte to sign extend
  data |= (int32_t)rbuffer[1] << 24;

  data |= (int32_t)rbuffer[2] << 16;

  data |= (int32_t)rbuffer[3] << 8;

  return data >> 8;
}

//writes data to an address
void adc_write(uint8_t addr, uint8_t data){
  HAL_StatusTypeDef status;
  uint8_t tbuffer[2] = {0};

  // modifying address to datasheet format
  addr <<= 3; 
  addr &= 0xF8; // setting bit 2 (READ_EN) to 0
  tbuffer[0] = addr;

  //adding data to buffer
  tbuffer[1] = data;

  status = adc_spi_tx(tbuffer, 2);

  if (status !=HAL_OK){
    printf("---Error writing to ADC! Status code: %d\n", status);
  }
}

uint32_t adc_read_current(void) {
  return adc_read(ADDR_CURRENT);
}

uint32_t adc_read_v1(void) {
  return adc_read(ADDR_V1);
}

uint32_t adc_read_v2(void) {
  return adc_read(ADDR_V2);
}
