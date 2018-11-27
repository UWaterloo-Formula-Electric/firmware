#include "bsp.h"
#include "PDU_can.h"
#include "state_machine.h"
#include "controlStateMachine.h"

void CAN_Msg_VCU_EM_Power_State_Request_Callback() {
    if (EM_Power_State_Request) {
        fsmSendEventISR(&motorFsmHandle, MTR_EV_EM_ENABLE);
    } else {
        fsmSendEventISR(&motorFsmHandle, MTR_EV_EM_DISABLE);
    }
}

void CAN_Msg_BMU_HV_Power_State_Callback() {
    if (HV_Power_State) {
        fsmSendEventISR(&coolingFsmHandle, COOL_EV_HV_ENABLE);
    } else {
        fsmSendEventISR(&coolingFsmHandle, COOL_EV_HV_DISABLE);
    }
}

void DTC_Fatal_Callback(BoardNames_t board) {
    fsmSendEventISR(&mainFsmHandle, MN_EV_HV_CriticalFailure);
}

