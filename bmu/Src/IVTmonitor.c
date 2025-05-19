#include "IVTmonitor.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "watchdog.h"
#include "canReceive.h"
#include "bsp.h"
#include "bmu_can.h"
#include "bmu_dtc.h"
#include "debug.h"



#define IVT_MONITOR_TASK_PERIOD_MS 1 
#define IVT_MONITOR_TIMEOUT_MS 500 // not sure what these values should be
void IVTmonitorTask()
{
    if (registerTaskToWatch(IVT_TASK_ID, pdMS_TO_TICKS(IVT_MONITOR_TASK_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register the IVT monitor task with watchdog \n");
        Error_Handler();
    }

    TickType_t LastWakeTick = xTaskGetTickCount(); 
    TickType_t LastRecievedTime = LastWakeTick;
    while(1)
    {
        vTaskDelayUntil(&LastWakeTick, IVT_MONITOR_TASK_PERIOD_MS);
        float currentIVT_U1 = 0.0f;
        // recieved a new value
        if (currentIVT_U1 != IVT_U1)
        {
            currentIVT_U1 = IVT_U1;
            LastRecievedTime = xTaskGetTickCount();
            watchdogTaskCheckIn(IVT_TASK_ID);
        }
        // timeout
        else if((xTaskGetTickCount() - LastRecievedTime) > pdMS_TO_TICKS(IVT_MONITOR_TIMEOUT_MS))
        {   
            sendDTC_ERROR_IVT_MSG_TIMEOUT();
        }
    }
}