/**
  *****************************************************************************
  * @file    ade7912.c
  * @author  Richard Matthews
  * @brief   Function to read HV ADC.
  * @details Functions to read the HV ADC. The HV ADC measures the HV bus
  * current, HV bus voltage, and HV battery voltage. The HV ADC is connected to
  * the BMU over SPI.
  *
  ******************************************************************************
  */

#include "bsp.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "math.h"

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

// Voltage scale and offset convert to volts
#define VOLTAGE_1_SCALE  (-0.000052670388F)
#define VOLTAGE_1_OFFSET (20.451204825373F)

#define VOLTAGE_2_SCALE  (-0.000052273823F)
#define VOLTAGE_2_OFFSET (23.442233510687)

/*#define VOLTAGE_1_SCALE  (1)*/
/*#define VOLTAGE_1_OFFSET (1)*/

/*#define VOLTAGE_2_SCALE  (1)*/
/*#define VOLTAGE_2_OFFSET (1)*/

// Current scale and offset current to volts, then we use shunt val to get
// current in amps
#define CURRENT_SCALE  (0.000000005332795540972819F/(1 + 1.3F))
#define CURRENT_OFFSET /*(-0.002105965908664F)*/ (-0.000916)
#define CURRENT_SHUNT_VAL_OHMS_HITL (0.000235F)
#define CURRENT_SHUNT_VAL_OHMS_CAR (0.0001F)

#define MAX_CURRENT_ADC_VOLTAGE (0.03125F)

// If we are on the hitl, there is a different current shunt resistor
extern bool HITL_Precharge_Mode;

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
uint8_t adc_readbyte(uint8_t addr, uint8_t *dataOut) {
  HAL_StatusTypeDef status;
  uint8_t rbuffer[2] = {0};
  uint8_t tbuffer[2] = {0};

  //modifying address to datasheet format
  addr <<= 3;
  addr |= 0x4; // Read Enable
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
  addr |= 0x4;
  tbuffer[0] = addr;

  status = adc_spi_tx_rx(tbuffer, (uint8_t *)rbuffer, 4);

  if (status != HAL_OK){
    ERROR_PRINT("---Error reading from ADC! Status code: %d\n", status);
    return HAL_ERROR;
  }

  /*DEBUG_PRINT("0: 0x%X, 1: 0x%X, 2: 0x%X\n", rbuffer[1], rbuffer[2], rbuffer[3]);*/
  //put data into an u32. Skipping rbuffer[0] bc it contains high impedance GARBAGE


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
  addr &= 0xF8; // setting bit 2 (READ_EN) to 0
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

  /*DEBUG_PRINT("%ld, ", raw);*/

  float shuntVoltage = CURRENT_SCALE * ((float)raw);
  /*DEBUG_PRINT("%.12f, ", shuntVoltage);*/
  shuntVoltage += CURRENT_OFFSET;
  /*DEBUG_PRINT("%.12f, ", shuntVoltage);*/

  if (fabs(shuntVoltage) > MAX_CURRENT_ADC_VOLTAGE) {
    ERROR_PRINT("IBus outside adc range!\n");
    *dataOut = 0;
    return HAL_ERROR;
  }

  if (HITL_Precharge_Mode) {
    shuntVoltage /= (CURRENT_SHUNT_VAL_OHMS_HITL);
  } else {
    shuntVoltage /= (CURRENT_SHUNT_VAL_OHMS_CAR);
  }
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
