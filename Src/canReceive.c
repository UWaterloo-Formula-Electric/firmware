#include "canReceive.h"

#include "userCan.h"
#include "bsp.h"
#include "debug.h"
#include "mainTaskEntry.h"

#include "DCU_can.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "DCU_dtc.h"

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

void CAN_Msg_BMU_DTC_Callback(int DTC_CODE, int DTC_Severity, int DTC_Data)
{
    if (DTC_CODE == FATAL_IMD_Failure) {
        ERROR_PRINT_ISR("Got IMD failure\n");
        IMD_FAIL_LED_ON
    } else if (DTC_CODE == CRITICAL_CELL_VOLTAGE_LOW
               || DTC_CODE == CRITICAL_CELL_VOLTAGE_HIGH
               || DTC_CODE == CRITICAL_CELL_TEMP_HIGH) {
        ERROR_PRINT_ISR("Got AMS failure\n");
        AMS_FAIL_LED_ON
    }
}

