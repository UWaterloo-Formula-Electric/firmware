#ifndef DCU_STATE_MACHINE
#define DCU_STATE_MACHINE
#include "state_machine.h"

typedef enum DCU_States_t {
    STATE_HV_Disable = 0U,
    STATE_HV_Enable,
    STATE_EM_Enable,
    STATE_Failure_Fatal,
    STATE_ANY,
} DCU_States_t;

typedef enum DCU_Events_t {
    EV_BTN_HV_Toggle = 0,
    EV_BTN_EM_Toggle,
    EV_BTN_TC_Toggle,
    EV_BTN_Endurance_Mode_Toggle,
    EV_BTN_Endurance_Lap,
    EV_CAN_Receive_HV,
    EV_CAN_Receive_EM,
    EV_Fatal,
    EV_ANY,
} DCU_Events_t;

HAL_StatusTypeDef dcuFsmInit();
bool pendingHvResponse(void);
bool pendingEmResponse(void);
void receivedHvResponse(void);
void receivedEmResponse(void);


extern FSM_Handle_Struct DCUFsmHandle;
#endif
