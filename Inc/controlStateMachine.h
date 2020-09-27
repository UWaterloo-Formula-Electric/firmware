#ifndef DCU_STATE_MACHINE
#define DCU_STATE_MACHINE
#include "state_machine.h"

typedef enum DCU_States_t {
    STATE_HV_Disable = 0,
    STATE_HV_Toggle,
    STATE_HV_Enable,
    STATE_EM_Toggle,
    STATE_EM_Enable,
    STATE_ANY,
} DCU_States_t;
typedef enum DCU_Events_t {
    EV_HV_Toggle = 0,
    EV_EM_Toggle,
    EV_CAN_Recieve,
    EV_ANY,
} DCU_Events_t;
FSM_Handle_Struct DCUFsmHandle;
HAL_StatusTypeDef dcuFsmInit();

#endif