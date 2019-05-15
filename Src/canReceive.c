#include "canReceive.h"

#include "userCan.h"
#include "bsp.h"
#include "debug.h"
#include "boardTypes.h"

#include "controlStateMachine.h"

#include "BMU_can.h"

#include "freertos.h"
#include "task.h"
#include "cmsis_os.h"


void CAN_Msg_DCU_buttonEvents_Callback()
{
    if (ButtonHVEnabled) {
        fsmSendEventISR(&fsmHandle, EV_HV_Toggle);
    }
}

void DTC_Fatal_Callback(BoardIDs board)
{
    fsmSendEventUrgentISR(&fsmHandle, EV_HV_Fault);
}

uint32_t lastChargeCartHeartbeat = 0;
bool sentChargeStartEvent = false;

void CAN_Msg_ChargeCart_heartbeat_Callback()
{
    if (!sentChargeStartEvent) {
        fsmSendEventISR(&fsmHandle, EV_Enter_Charge_Mode);
        sentChargeStartEvent = true;
    }
    lastChargeCartHeartbeat = xTaskGetTickCount();
}

