#include "canReceive.h"

#include "userCan.h"
#include "bsp.h"
#include "debug.h"

#include "drive_by_wire.h"

#include "VCU_F7_can.h"

#include "freertos.h"
#include "task.h"
#include "cmsis_os.h"

extern osThreadId driveByWireHandle;

void CAN_Msg_DCU_buttonEvents_Callback()
{
    if (ButtonEMEnabled) {
        fsmSendEventISR(&fsmHandle, EV_EM_Toggle);
    }
    // For now, ignore HV Enable button, as we really want to wait for BMU to
    // complete HV Enable
}

volatile bool motorControllersStatus = false;

void CAN_Msg_PDU_ChannelStatus_Callback()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (StatusPowerMCLeft == StatusPowerMCLeft_CHANNEL_ON &&
        StatusPowerMCRight == StatusPowerMCRight_CHANNEL_ON) {
        xTaskNotifyFromISR( driveByWireHandle,
                            (1<<NTFY_MCs_ON),
                            eSetBits,
                            &xHigherPriorityTaskWoken );
        motorControllersStatus = true;
    } else if (motorControllersStatus) {
        // Only send a notification it MCs turned off if MCs were already ON
        xTaskNotifyFromISR( driveByWireHandle,
                            (1<<NTFY_MCs_OFF),
                            eSetBits,
                            &xHigherPriorityTaskWoken );
    }

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
