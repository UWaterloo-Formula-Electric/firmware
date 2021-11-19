#ifndef CAN_RECEIVE_H_
#define CAN_RECEIVE_H_

#include "stdbool.h"
#include "bsp.h"

bool getHvEnableState();
bool getMotorControllersStatus();
uint32_t lastBrakeValReceiveTimeTicks;

#endif /* USER_CAN_H_ */
