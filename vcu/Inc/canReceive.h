#ifndef CAN_RECEIVE_H_
#define CAN_RECEIVE_H_

#include "stdbool.h"
#include "bsp.h"

bool getHvEnableState();
bool getMotorControllersStatus();
bool isLockoutDisabled();
uint8_t getInverterVSMState();
uint64_t getInverterFaultCode();

volatile uint8_t inverterVSMState;
volatile uint8_t inverterInternalState;

#endif /* USER_CAN_H_ */
