#ifndef FAULTMONITOR_H

#define FAULTMONITOR_H

#include "bsp.h"


HAL_StatusTypeDef HVIL_Control(bool enable);
bool getHVIL_Status();
bool getIL_Status();
bool getBSPD_Status();
bool getTSMS_Status();
bool getHVD_Status();

#endif /* end of include guard: FAULTMONITOR_H */
