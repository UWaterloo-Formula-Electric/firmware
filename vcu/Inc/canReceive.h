#ifndef CAN_RECEIVE_H_
#define CAN_RECEIVE_H_

#include "stdbool.h"
#include "bsp.h"

bool getHvEnableState();
bool getMotorControllersStatus();
bool isLockoutDisabled();
bool getHVState();
bool getEMState();
uint8_t getInverterVSMState();

volatile uint8_t inverterVSMState;
volatile uint8_t inverterInternalState;

#endif /* USER_CAN_H_ */
