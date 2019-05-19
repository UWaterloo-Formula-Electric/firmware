#include "canReceive.h"

#include "userCan.h"
#include "bsp.h"
#include "debug.h"
#include "mainTaskEntry.h"

#include "DCU_can.h"

#include "freertos.h"
#include "cmsis_os.h"

extern osThreadId mainTaskHandle;

bool getHVState()
{
    return HV_Power_State;
}

bool getEMState()
{
    return EM_State;
}

void CAN_Msg_BMU_HV_Power_State_Callback()
{
    if (HV_Power_State == HV_Power_State_On) {
        xTaskNotifyFromISR( mainTaskHandle,
                            (1<<HV_ENABLED_NOTIFICATION),
                            eSetBits,
                            NULL );
    } else {
        xTaskNotifyFromISR( mainTaskHandle,
                            (1<<HV_DISABLED_NOTIFICATION),
                            eSetBits,
                            NULL );
    }
}

void CAN_Msg_VCU_EM_State_Callback()
{
    if (EM_State == HV_Power_State_On) {
        xTaskNotifyFromISR( mainTaskHandle,
                            (1<<EM_ENABLED_NOTIFICATION),
                            eSetBits,
                            NULL );
    } else {
        xTaskNotifyFromISR( mainTaskHandle,
                            (1<<EM_DISABLED_NOTIFICATION),
                            eSetBits,
                            NULL );
    }
}
