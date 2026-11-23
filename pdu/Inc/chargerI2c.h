#ifndef CHARGERI2C_H 
#define CHARGERI2C_H

#include "stm32f7xx_hal.h"

// Reads one register, single attempt, no retry (team rule): any failure returns HAL_ERROR.
// Worst case (bus dead): busy-waits 2 ms (the HAL timeout). Never call inside a critical section.
HAL_StatusTypeDef chargerReadRegister(uint8_t reg, uint8_t *data);

// Writes one register, single attempt, no retry: any failure returns HAL_ERROR.
HAL_StatusTypeDef chargerWriteRegister(uint8_t reg, uint8_t data);

// 16 bit versions for registers that span reg and reg + 1, done in one I2C transaction.
// Little endian per datasheet. Same single attempt rules as the 8 bit versions.
HAL_StatusTypeDef chargerRead16BitRegister(uint8_t reg, uint16_t *data);
HAL_StatusTypeDef chargerWrite16BitRegister(uint8_t reg, uint16_t data);

// read modify write, value must already be shifted into position
HAL_StatusTypeDef chargerModifyBits(uint8_t reg, uint8_t mask, uint8_t value);

#endif  // CHARGERI2C_H 
