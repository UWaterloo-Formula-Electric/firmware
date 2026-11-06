#ifndef CHARGERI2C_H 
#define CHARGERI2C_H

#include "stm32f7xx_hal.h"

// Reads one register retries on bus failure
// Worst case (bus dead): blocks 23 ms = attempts × timeout + (attempts − 1) × delay.
// Uses vTaskDelay between retries, so it must be called from a task, not before the scheduler starts.
HAL_StatusTypeDef chargerReadRegister(uint8_t reg, uint8_t *data);

//wrapper for Hal_I2C_Mem_Write to write a single byte to a register on the charger, doesn't retry 
HAL_StatusTypeDef chargerWriteRegister(uint8_t reg, uint8_t data);

// read modify write
HAL_StatusTypeDef chargerModifyBits (uint8_t reg, uint8_t mask, uint8_t value);



#endif  // CHARGERI2C_H 
