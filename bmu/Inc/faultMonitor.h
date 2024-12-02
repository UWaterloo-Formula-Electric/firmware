#ifndef FAULTMONITOR_H

#define FAULTMONITOR_H

#include "bsp.h"


bool getHVIL_Status();
bool getIL_Status();
bool getBSPD_Status();
bool getTSMS_Status();
bool getHVD_Status();
bool getIL_BRB_Status();
bool getCBRB_IL_Status();

#endif /* end of include guard: FAULTMONITOR_H */
