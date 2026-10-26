#ifndef CHARGERI2C_H 
#define CHARGERI2C_H

#include "stm32f7xx_hal.h"

HAL_StatusTypeDef chargerReadRegister(uint8_t reg, uint8_t *data);
// Reads one register retries on bus failure
// Worst case (bus dead): blocks 23 ms = attempts × timeout + (attempts − 1) × delay.
// Uses vTaskDelay between retries, so it must be called from a task, not before the scheduler starts.

#endif  // CHARGERI2C_H 
