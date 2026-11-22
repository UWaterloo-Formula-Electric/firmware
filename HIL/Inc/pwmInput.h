#ifndef PWM_INPUT_H
#define PWM_INPUT_H
#define PWM_READ_TIMEOUT_MS 10U

#include "stm32f7xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

HAL_StatusTypeDef pwmInputInit(void);
bool pwmInputRead(uint16_t *period, uint16_t *high);
uint32_t pwmInputCounterClockHz(void);

#endif