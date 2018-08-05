#ifndef __DRIVE_BY_WIRE_MOCK_H
#define __DRIVE_BY_WIRE_MOCK_H
#include "stm32f7xx_hal.h"
#include "stdbool.h"

HAL_StatusTypeDef outputThrottle();
int getThrottle();
bool checkBPSState();
int getBrakePressure();
bool throttle_is_zero();
bool getHvEnableState();

#endif // __DRIVE_BY_WIRE_MOCK_H
