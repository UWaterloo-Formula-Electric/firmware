#include "bsp.h"
#include "PDU_can.h"
#include "PDU_dtc.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "debug.h"

void CAN_Msg_VCU_EM_Power_State_Request_Callback() {
    if (EM_Power_State_Request) {
        fsmSendEventISR(&motorFsmHandle, MTR_EV_EM_ENABLE);
    } else {
        fsmSendEventISR(&motorFsmHandle, MTR_EV_EM_DISABLE);
    }
}

void CAN_Msg_BMU_HV_Power_State_Callback() {
    if (HV_Power_State == HV_Power_State_On) {
        fsmSendEventISR(&coolingFsmHandle, COOL_EV_HV_ENABLE);
    } else {
        fsmSendEventISR(&coolingFsmHandle, COOL_EV_HV_DISABLE);
    }
}

void DTC_Fatal_Callback(BoardNames_t board) {
    fsmSendEventUrgentISR(&mainFsmHandle, MN_EV_HV_CriticalFailure);
}

void CAN_Msg_BMU_DTC_Callback(int DTC_CODE, int DTC_Severity, int DTC_Data) {
    switch (DTC_CODE) {
        case WARNING_CELL_TEMP_HIGH:
            fsmSendEventISR(&coolingFsmHandle, COOL_EV_OVERTEMP_WARNING);
            break;
        default:
            // Do nothing, other events handled by fatal callback
            break;
    }
}
