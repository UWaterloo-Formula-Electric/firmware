#include "canReceive.h"

#include "userCan.h"
#include "bsp.h"
#include "debug.h"

#include "drive_by_wire.h"

#include "VCU_F7_can.h"

#include "freertos.h"

void CAN_Msg_DCU_buttonEvents_Callback()
{
    if (ButtonEMEnabled) { // TODO: This is a float comparison, is it safe?
        fsmSendEventISR(&fsmHandle, EV_EM_Toggle);
    }
    // For now, ignore HV Enable button, as we really want to wait for BMU to
    // complete HV Enable
}

void canReceive(const void *pvParameters)
{
    canStart(&CAN_HANDLE);

    DEBUG_PRINT("Starting CAN Receive task\n");

    while (1)
    {

    }
}
