#ifndef MOTORCONTROLLER_H

#define MOTORCONTROLLER_H

#include "bsp.h"

HAL_StatusTypeDef mcInit();
HAL_StatusTypeDef sendThrottleValueToMCs(float throttle);
HAL_StatusTypeDef mcShutdown();

#endif /* end of include guard: MOTORCONTROLLER_H */
