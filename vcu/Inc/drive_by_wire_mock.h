#ifndef __DRIVE_BY_WIRE_MOCK_H
#define __DRIVE_BY_WIRE_MOCK_H
#include "stm32f7xx_hal.h"
#include "stdbool.h"

#define MIN_BRAKE_PRESSURE_PSI 250 // For sensor M3031-000005-2K5PG, we saw a max psi of 1875, this is approximately 13% of that

HAL_StatusTypeDef stateMachineMockInit();

#endif // __DRIVE_BY_WIRE_MOCK_H
