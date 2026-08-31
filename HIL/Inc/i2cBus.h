/**
  *****************************************************************************
  * @file    i2cBus.h
  * @brief   Generic transfer wrappers for the bench I2C buses
  *****************************************************************************
  */

#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "stm32f7xx_hal.h"

typedef enum {
    I2C_BUS_1 = 0,
    I2C_BUS_2,
    I2C_BUS_3,
    NUM_I2C_BUSES
} I2cBus_t;

// devAddress is the 7 bit address shifted left by one, as the HAL expects
HAL_StatusTypeDef i2cWrite(I2cBus_t bus, uint16_t devAddress, uint8_t *data, uint16_t length);
HAL_StatusTypeDef i2cRead(I2cBus_t bus, uint16_t devAddress, uint8_t *data, uint16_t length);
HAL_StatusTypeDef i2cWriteReg(I2cBus_t bus, uint16_t devAddress, uint8_t reg, uint8_t *data, uint16_t length);
HAL_StatusTypeDef i2cReadReg(I2cBus_t bus, uint16_t devAddress, uint8_t reg, uint8_t *data, uint16_t length);
HAL_StatusTypeDef i2cIsDeviceReady(I2cBus_t bus, uint16_t devAddress);

#endif /* I2C_BUS_H */
