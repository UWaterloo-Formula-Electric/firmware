#ifndef __DRIVE_BY_WIRE_H
#define __DRIVE_BY_WIRE_H
#include "stm32f7xx_hal.h"
#include "state_machine.h"

typedef enum VCU_States_t {
    STATE_Boards_Off = 0,
    STATE_Boards_On,
    STATE_Warning_Critical,
    STATE_Critical_Failure,
    STATE_ANY, // Must be the last state
} Main_PDU_States_t;

typedef enum VCU_Events_t {
    EV_Init = 0,
    EV_HV_CriticalFailure,
    EV_CriticalDelayElapsed,
    EV_LV_Cuttoff,
    EV_ANY, // Must be the last event
} MAIN_PDU_Events_t;

extern FSM_Handle_Struct mainFsmHandle;

HAL_StatusTypeDef initStateMachines();
HAL_StatusTypeDef startMainControl();
void mainControlTask(void *pvParameters);
#endif // __DRIVE_BY_WIRE_H
