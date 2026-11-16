#ifndef __DRIVE_BY_WIRE_MOCK_H
#define __DRIVE_BY_WIRE_MOCK_H
#include "stm32f7xx_hal.h"
#include "stdbool.h"

#define MIN_BRAKE_PRESSURE_PSI 250 // For 5000 PSI sensor (MSP300), 250 PSI is 5% threshold to confirm hard brake press

HAL_StatusTypeDef stateMachineMockInit();

#endif // __DRIVE_BY_WIRE_MOCK_H
