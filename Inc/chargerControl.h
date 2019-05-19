#ifndef CHARGERCONTROL_H

#define CHARGERCONTROL_H

#include "bsp.h"

HAL_StatusTypeDef sendChargerCommand(float maxVoltage, float maxCurrent, bool startCharging);

#endif /* end of include guard: CHARGERCONTROL_H */
