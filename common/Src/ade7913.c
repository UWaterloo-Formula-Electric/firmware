/**
  *****************************************************************************
  * @file    ade7913.c
  * @author  Daniel Bishara
  * @brief   Function to read HV ADC.
  * @details Functions to read the cell Tester ADC. The cell tester ADC measures
  * current, and the cell voltage. ADC is connected to the cell tester over SPI.
  * These functions were taken from the BMU as it uses the same ADC, but scales
  * and offsets were modified to accodomate for different resistor values.
  *
  ******************************************************************************
  */

#include "ade7913.h"
#include "bsp.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "math.h"
#include "main.h"

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


#define MAX_CURRENT_ADC_VOLTAGE (0.03125F)

#define READ_EN 0x4
#define WRITE_EN 0xF8

HAL_StatusTypeDef adc_spi_tx(uint8_t * tdata, unsigned int len) {
    HAL_GPIO_WritePin(ISO_ADC_CS_GPIO_Port, ISO_ADC_CS_Pin , GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&ISO_ADC_SPI_HANDLE, tdata, len, SPI_TIMEOUT);
    HAL_GPIO_WritePin(ISO_ADC_CS_GPIO_Port, ISO_ADC_CS_Pin , GPIO_PIN_SET);
    return status;
}

HAL_StatusTypeDef adc_spi_tx_rx(uint8_t * tdata, uint8_t * rbuffer, unsigned int len) {
    HAL_GPIO_WritePin(ISO_ADC_CS_GPIO_Port, ISO_ADC_CS_Pin , GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&ISO_ADC_SPI_HANDLE, tdata, rbuffer, len, SPI_TIMEOUT);
    HAL_GPIO_WritePin(ISO_ADC_CS_GPIO_Port, ISO_ADC_CS_Pin , GPIO_PIN_SET);
    return status;
}

// also return data
uint8_t adc_readbyte(uint8_t addr, uint8_t *dataOut) {
    HAL_StatusTypeDef status;
    uint8_t rbuffer[2] = {0};
    uint8_t tbuffer[2] = {0};

    // modifying address to datasheet format
    addr <<= 3;
    addr |= READ_EN;  // Read Enable
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
    shuntVoltage /= (CURRENT_SHUNT_VAL_OHMS);

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

  //DEBUG_PRINT("raw v2: %ld\r\n", raw);
  (*dataOut) = VOLTAGE_2_SCALE*((float)raw + VOLTAGE_2_OFFSET);
  return HAL_OK;
}

HAL_StatusTypeDef adc_read_v(float *dataOut){
    float v1, v2;
    if (adc_read_v1(&v1) != HAL_OK) {
        return HAL_ERROR;
    }
    if (adc_read_v2(&v2) != HAL_OK) {
        return HAL_ERROR;
    }

    (*dataOut) = v2-v1;
    return HAL_OK;
}

HAL_StatusTypeDef hvadc_init() {
    uint8_t status0;

    // Wait for ADC to come out of reset
    do {
        if (adc_readbyte(ADDR_STATUS0, &status0) != HAL_OK) {
            ERROR_PRINT("Error reading HV ADC status register\n");
            return HAL_ERROR;
        }
        // ERROR_PRINT("waiting for hv ADC on bit\r\n");
        vTaskDelay(5);
    } while (status0 & (1 << RESET_ON_BIT));

    // Set up config register
    uint8_t cfgWrite = ADC_LOW_BW_ENABLE | ADC_FREQ_1KHZ;
    if (adc_write(ADDR_CFG, cfgWrite) != HAL_OK) {
        ERROR_PRINT("Failed to config HV ADC\n");
        return HAL_ERROR;
    }
    ERROR_PRINT("waiting for ADC on\r\n");
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

void fetControlTask(void *pvParamaters)
{
    while(1)
    {
        vTaskDelay(10000);
    }
}

void temperatureTask(void *pvParamaters)
{
    hvadc_init();
    HAL_StatusTypeDef v1_ret;
    HAL_StatusTypeDef v2_ret;
    HAL_StatusTypeDef I_ret;
    float v1 = 0.0f;
    float v2 = 0.0f;
    float I = 0.0f;

    while (1)
    {
        v1_ret = adc_read_v1(&v1);
        v2_ret = adc_read_v2(&v2);
        I_ret = adc_read_current(&I);
        if (v1_ret != HAL_OK)
        {
            DEBUG_PRINT("v1 error\r\n");
        }
        else if (v2_ret != HAL_OK)
        {
            DEBUG_PRINT("v2 error\r\n");
        }
        else if (I_ret != HAL_OK)
        {
            DEBUG_PRINT("I error\r\n");
        }
        else
        {
            DEBUG_PRINT("v1: %f, v2: %f, I: %f\r\n", v1, v2, I);
        }
        
        vTaskDelay(500);
    }

    return HAL_OK;
}
