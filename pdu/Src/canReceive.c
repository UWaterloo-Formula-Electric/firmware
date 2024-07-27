#include "bsp.h"
#include "pdu_can.h"
#include "pdu_dtc.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "uwfe_debug.h"
#include "boardTypes.h"
#include "canReceive.h"

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

volatile uint8_t resetting = 0U;
volatile uint64_t inverterFaultCode = 0U;
void CAN_Msg_MC_Fault_Codes_Callback() // 100 hz
{
    // Each bit represents a fault
    // Combine them to be sent over DTCs
    inverterFaultCode = (INV_Post_Fault_Hi << 48) | (INV_Post_Fault_Lo << 32) | (INV_Run_Fault_Hi << 16) | INV_Run_Fault_Lo;
    if (inverterFaultCode && !resetting)
    {
        resetting = 1;
        fsmSendEventUrgentISR(&mainFsmHandle, EV_Cycle_MC);
    }
}