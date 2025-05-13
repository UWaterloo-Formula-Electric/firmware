#include "brakeLight.h"
#include "bsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pdu_can.h"
#include "debug.h"

bool isBrakePressed(uint32_t brakePercent)
{
    return (BrakePercent > BRAKE_LIGHT_ON_THRESHOLD);
}

void CAN_Msg_VCU_Data_Callback()
{
    /* Added hystersis to prevent flickering */
    if (BrakePercent > BRAKE_LIGHT_ON_THRESHOLD) {
        BRAKE_LIGHT_ENABLE;
    } else if (BrakePercent < BRAKE_LIGHT_OFF_THRESHOLD){
        BRAKE_LIGHT_DISABLE;
    }
}