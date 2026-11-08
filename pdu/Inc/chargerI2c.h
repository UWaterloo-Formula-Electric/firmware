#ifndef CHARGERI2C_H 
#define CHARGERI2C_H

#include "stm32f7xx_hal.h"

// Reads one register retries on bus failure
// Worst case (bus dead): blocks 18 ms = attempts × timeout + (attempts − 1) × delay.
// Uses vTaskDelay between retries, so it must be called from a task, not before the scheduler starts.
HAL_StatusTypeDef chargerReadRegister(uint8_t reg, uint8_t *data);

// Writes one register, single attempt with no retry: some registers have self-clearing
// command bits, so the caller decides whether a retry is safe.
HAL_StatusTypeDef chargerWriteRegister(uint8_t reg, uint8_t data);

// 16 bit versions for registers that span reg and reg + 1, done in one I2C transaction.
// Little endian per datasheet. Same retry rules as the 8 bit versions.
HAL_StatusTypeDef chargerRead16BitRegister(uint8_t reg, uint16_t *data);
HAL_StatusTypeDef chargerWrite16BitRegister(uint8_t reg, uint16_t data);

// read modify write, value must already be shifted into position
HAL_StatusTypeDef chargerModifyBits(uint8_t reg, uint8_t mask, uint8_t value);

#endif  // CHARGERI2C_H 
