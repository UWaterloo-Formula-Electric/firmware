#ifndef FANCONTROL_H

#define FANCONTROL_H

#include "stdbool.h"

void setManualFanOverride(bool override);
HAL_StatusTypeDef fanInit();
HAL_StatusTypeDef setFan();
void fanTask();

#endif /* end of include guard: FANCONTROL_H */
