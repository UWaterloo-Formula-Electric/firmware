#ifndef CAN_RECEIVE_H_
#define CAN_RECEIVE_H_

#include "stdbool.h"
#include "bsp.h"

typedef struct mcParameterResponse {
    uint16_t returnedAddress;
    bool writeSuccess;
    uint16_t returnedData;
} mcParameterResponse;

bool getHvEnableState();
bool getMotorControllersStatus();
bool isLockoutDisabled();
uint8_t getInverterVSMState();
uint64_t getInverterFaultCode();
void getMcParamResponse(mcParameterResponse *buffer);

volatile uint8_t inverterVSMState;
volatile uint8_t inverterInternalState;

#endif /* USER_CAN_H_ */
