#ifndef __EEPROM_H
#define __EEPROM_H

#include "stm32f7xx_hal.h"
#include "i2c.h"
#include <string.h>

/* AT24C02C I2C address */
#define EEPROM_I2C_ADDRESS    (0x50U << 1)

/* EEPROM configuration */
#define EEPROM_SIZE_BYTES  256U
#define EEPROM_PAGE_SIZE   8U

/* EEPROM driver initialization */
HAL_StatusTypeDef EEPROM_Init(void);

/* EEPROM operations */
HAL_StatusTypeDef EEPROM_Erase(void);
HAL_StatusTypeDef EEPROM_Write(uint8_t address, const uint8_t *data, uint16_t length);
HAL_StatusTypeDef EEPROM_Read(uint8_t address, uint8_t *data, uint16_t length);

#endif /* __EEPROM_H */
