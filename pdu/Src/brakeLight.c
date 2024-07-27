#include "brakeLight.h"
#include "bsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pdu_can.h"
#include "uwfe_debug.h"

#define BRAKE_LIGHT_ON_THRESHOLD 15
#define BRAKE_TASK_PERIOD_MS 300

bool isBrakePressed(uint32_t brakePercent)
{
    return (BrakePercent > BRAKE_LIGHT_ON_THRESHOLD);
}

void CAN_Msg_VCU_Data_Callback()
{
    if (isBrakePressed(BrakePercent)) {
        BRAKE_LIGHT_ENABLE;
    } else {
        BRAKE_LIGHT_DISABLE;
    }
}
