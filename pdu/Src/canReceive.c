#include "bsp.h"
#include "pdu_can.h"
#include "pdu_dtc.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "boardTypes.h"
#include "canReceive.h"

volatile uint32_t MC_Last_Temperature_Msg_ticks = 0;

void CAN_Msg_UartOverCanConfig_Callback() {
    isUartOverCanEnabled = UartOverCanConfigSignal & 0x4;
}

void CAN_Msg_VCU_EM_Power_State_Request_Callback() {
    if (EM_Power_State_Request == EM_Power_State_Request_On) {
        fsmSendEventISR(&mainFsmHandle, EV_EM_Enable);
    } else {
        fsmSendEventISR(&mainFsmHandle, EV_EM_Disable);
    }
}

void DTC_Fatal_Callback(BoardIDs board) {
    DEBUG_PRINT_ISR("DTC Receieved from board %lu \n", board);
    fsmSendEventUrgentISR(&mainFsmHandle, EV_HV_CriticalFailure);
}

void CAN_Msg_MC_Temperature_Set_3_Callback() { // 10hz
    MC_Last_Temperature_Msg_ticks = xTaskGetTickCountFromISR();
}
