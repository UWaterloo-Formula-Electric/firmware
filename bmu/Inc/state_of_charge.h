#ifndef STATE_OF_CHARGE_H
#define STATE_OF_CHARGE_H
#include "FreeRTOS.h"
#include "stm32f7xx_hal.h"

void integrate_bus_current(float IBus, float period_ms);
HAL_StatusTypeDef socInit(void);

HAL_StatusTypeDef setCapacityStartup(float capacity);
void setIBusIntegrated(float input);

float getCapacityStartup(void);
float getIBusIntegrated(void);

#endif
