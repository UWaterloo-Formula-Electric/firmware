/**
  *****************************************************************************
  * @file    ade7913.c
  * @author  Richard Matthews
  * @brief   Function to read HV ADC.
  * @details Functions to read the HV ADC. The HV ADC measures the HV bus
  * current, HV bus voltage, and HV battery voltage. The HV ADC is connected to
  * the cell tester over SPI.
  *
  ******************************************************************************
  */

#include "bsp.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "math.h"
#include "main.h"
#include "ade7913.h"

HAL_StatusTypeDef adc_spi_tx(uint8_t * tdata, unsigned int len) {
    HAL_GPIO_WritePin(ISO_ADC_CS_GPIO_Port, ISO_ADC_CS_Pin , GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&ISO_ADC_SPI_HANDLE, tdata, len, SPI_TIMEOUT);
    HAL_GPIO_WritePin(ISO_ADC_CS_GPIO_Port, ISO_ADC_CS_Pin , GPIO_PIN_SET);
    return status;
}

HAL_StatusTypeDef adc_spi_tx_rx(uint8_t * tdata, uint8_t * rbuffer, unsigned int len) {
    HAL_GPIO_WritePin(ISO_ADC_CS_GPIO_Port, ISO_ADC_CS_Pin , GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&HV_ADC_SPI_HANDLE, tdata, rbuffer, len, SPI_TIMEOUT);
    HAL_GPIO_WritePin(ISO_ADC_CS_GPIO_Port, ISO_ADC_CS_Pin , GPIO_PIN_SET);
    return status;
}

// also return data
uint8_t adc_readbyte(uint8_t addr, uint8_t *dataOut) {
  HAL_StatusTypeDef status;
  uint8_t rbuffer[2] = {0};
  uint8_t tbuffer[2] = {0};

  //modifying address to datasheet format
  //should make macros for masks
  addr <<= 3;
  addr |= READ_EN; // Read Enable
  tbuffer[0] = addr;

  status = adc_spi_tx_rx(tbuffer, rbuffer, 2);
  if (status != HAL_OK) {
    ERROR_PRINT("Error reading HV ADC\n");
    return HAL_ERROR;
  }

  (*dataOut) = rbuffer[1];
  return HAL_OK;
}

//return the data from the address
HAL_StatusTypeDef adc_read(uint8_t addr, int32_t *dataOut){
  HAL_StatusTypeDef status;
  uint8_t rbuffer[4] = {0};
  uint8_t tbuffer[4] = {0};
  int32_t data = 0x0;

  //modifying address to datasheet format
  addr <<= 3;
  addr |= READ_EN;
  tbuffer[0] = addr;

  status = adc_spi_tx_rx(tbuffer, (uint8_t *)rbuffer, 4);

  if (status != HAL_OK){
    ERROR_PRINT("---Error reading from ADC! Status code: %d\n", status);
    return HAL_ERROR;
  }

  // shift to upper three bytes, then right-shift one byte to sign extend
  data |= (int32_t)rbuffer[1] << 24;

  data |= (int32_t)rbuffer[2] << 16;

  data |= (int32_t)rbuffer[3] << 8;

  (*dataOut) = data >> 8;

  return HAL_OK;
}

//writes data to an address
HAL_StatusTypeDef adc_write(uint8_t addr, uint8_t data){
  HAL_StatusTypeDef status;
  uint8_t tbuffer[2] = {0};

  // modifying address to datasheet format
  addr <<= 3;
  addr &= WRITE_EN; // setting bit 2 (READ_EN) to 0
  tbuffer[0] = addr;

  //adding data to buffer
  tbuffer[1] = data;

  status = adc_spi_tx(tbuffer, 2);

  if (status != HAL_OK){
    ERROR_PRINT("Error writing to HVADC!\n");
    return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef adc_read_current(float *dataOut) {
  int32_t raw;
  if (adc_read(ADDR_CURRENT, &raw) != HAL_OK) {
    return HAL_ERROR;
  }

  float shuntVoltage = CURRENT_SCALE * ((float)raw);
  shuntVoltage += CURRENT_OFFSET;
  shuntVoltage /= (CURRENT_SHUNT_VAL_OHMS_CELL_TESTER);

  /*DEBUG_PRINT("%.12f\n", shuntVoltage);*/
  (*dataOut) = shuntVoltage;

  return HAL_OK;
}

HAL_StatusTypeDef adc_read_v1(float *dataOut) {
  int32_t raw;
  if (adc_read(ADDR_V1, &raw) != HAL_OK) {
    return HAL_ERROR;
  }

  (*dataOut) = (VOLTAGE_1_SCALE * ((float)raw)) + VOLTAGE_1_OFFSET;
  return HAL_OK;
}

HAL_StatusTypeDef adc_read_v2(float *dataOut) {
  int32_t raw;
  if (adc_read(ADDR_V2, &raw) != HAL_OK) {
    return HAL_ERROR;
  }

  (*dataOut) = (VOLTAGE_2_SCALE * ((float)raw)) + VOLTAGE_2_OFFSET;
  return HAL_OK;
}

HAL_StatusTypeDef hvadc_init()
{
  uint8_t status0;

  // Wait for ADC to come out of reset
  do {
    if (adc_readbyte(ADDR_STATUS0, &status0) != HAL_OK) {
      ERROR_PRINT("Error reading HV ADC status register\n");
      return HAL_ERROR;
    }
    vTaskDelay(5);
  } while (status0 & (1<<RESET_ON_BIT));

  // Set up config register
  uint8_t cfgWrite = ADC_LOW_BW_ENABLE | ADC_FREQ_1KHZ;
  if (adc_write(ADDR_CFG, cfgWrite) != HAL_OK)
  {
    ERROR_PRINT("Failed to config HV ADC\n");
    return HAL_ERROR;
  }

  // recomended by datasheet to read value after writing to double check
  uint8_t cfgRead;
  if (adc_readbyte(ADDR_CFG, &cfgRead) != HAL_OK) {
    ERROR_PRINT("Error reading HV ADC config register\n");
    return HAL_ERROR;
  }

  if (cfgRead != cfgWrite) {
    ERROR_PRINT("HVADC: config wrote %d != read %d\n", cfgWrite, cfgRead);
    return HAL_ERROR;
  }

  return HAL_OK;
}
