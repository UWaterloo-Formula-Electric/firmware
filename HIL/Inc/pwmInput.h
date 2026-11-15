#ifndef PWM_INPUT_H
#define PWN_INPUT_H

#include "stm32f7xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

HAL_StatusTypeDef pwmInputInit(void);
bool pwmInputRead(uint16_t *period, uint16_t *high);

#endif