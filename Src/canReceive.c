#include "canReceive.h"

#include "userCan.h"
#include "bsp.h"
#include "debug.h"

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

void DTC_Fatal_Callback(BoardNames_t board)
{
    fsmSendEventUrgentISR(&fsmHandle, EV_HV_Fault);
}
