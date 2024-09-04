#ifndef FANCONTROL_H

#define FANCONTROL_H

#include "stm32f7xx_hal.h"
#include <stdint.h>

HAL_StatusTypeDef setFanDutyCycle(uint8_t DC);

#endif /* end of include guard: FANCONTROL_H */
