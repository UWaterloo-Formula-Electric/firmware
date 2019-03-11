#ifndef __DRIVE_BY_WIRE_MOCK_H
#define __DRIVE_BY_WIRE_MOCK_H
#include "stm32f7xx_hal.h"
#include "stdbool.h"

#define MIN_BRAKE_PRESSURE 50 // TODO: Set this to a reasonable value

HAL_StatusTypeDef stateMachineMockInit();

#endif // __DRIVE_BY_WIRE_MOCK_H
