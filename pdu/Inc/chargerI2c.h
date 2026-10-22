#ifndef CHARGERI2C_H 
#define CHARGERI2C_H

#include "stm32f7xx_hal.h"

HAL_StatusTypeDef chargerReadRegister(uint8_t reg, uint8_t *data);

#endif  // CHARGERI2C_H 