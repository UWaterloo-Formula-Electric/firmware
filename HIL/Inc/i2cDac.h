/**
 * @file    i2cDac.h
 * @brief   Driver for MCP4728 chip 1 (I2C1, address 0x60, LDAC_1 on PF10)
 */

#ifndef I2C_DAC_H
#define I2C_DAC_H
 
#include "stm32f7xx_hal.h"
#include <stdint.h>
 
#define I2C_DAC_CODE_MAX 4095U   // 12-bit: 0 to 4095
 
// Park all four LDAC lines high so no chip loads until we pulse it.
// Call once from userInit(), after MX_GPIO_Init().
HAL_StatusTypeDef i2cDacInit(void);
 
// HAL_OK if the MCP4728 acknowledges its address on I2C1.
HAL_StatusTypeDef i2cDacProbe(void);
 
// Write codeA to channel A and codeB to channel B, then update both together.
// Uses vTaskDelay, so only call once FreeRTOS is running.
HAL_StatusTypeDef i2cDacSetCodes(uint16_t codeA, uint16_t codeB);
 
#endif /* I2C_DAC_H */
