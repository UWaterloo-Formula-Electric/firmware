#include "controlStateMachine.h"
#include "state_machine.h"
uint32_t toggleOnHV(uint32_t event);
uint32_t toggleOnEM(uint32_t event);
FSM_Handle_Struct DCUFsmHandle;

Transition_t transitions[] = {
    {STATE_HV_Disable,EV_HV_Toggle,&toggleOnHV},
    {STATE_HV_Toggle,EV_CAN_Recieve,},
    {STATE_HV_Enable, EV_EM_Toggle,&toggleOnEM},
    {STATE_EM_Toggle, EV_CAN_Recieve,},
    {STATE_EM_Enable, EV_EM_Toggle,},
    {STATE_HV_Enable, EV_HV_Toggle,}
}
uint32_t toggleOnHV(uint32_t event){
    DEBUG_PRINT("Sending hv changed\n");
    sendCAN_DCU_buttonEvents();
    return STATE_HV_Toggle;
}
uint32_t toggleOnEM(uint32_t event){
    DEBUG_PRINT("Sending hv changed\n");
    sendCAN_DCU_buttonEvents();
    return STATE_HV_Toggle;
}
