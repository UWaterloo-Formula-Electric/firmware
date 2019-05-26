#ifndef MOTORCONTROLLER_H

#define MOTORCONTROLLER_H

#include "bsp.h"

#define MAX_TORQUE_DEMAND_DEFAULT 100
#define SPEED_LIMIT_DEFAULT 100

extern uint64_t maxTorqueDemand;

HAL_StatusTypeDef mcInit();
HAL_StatusTypeDef sendThrottleValueToMCs(float throttle);
HAL_StatusTypeDef mcShutdown();
HAL_StatusTypeDef initSpeedAndTorqueLimits();

#endif /* end of include guard: MOTORCONTROLLER_H */
