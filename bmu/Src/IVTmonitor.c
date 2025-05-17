#include "IVTmonitor.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "watchdog.h"
#include "canReceive.h"
#include "bsp.h"
#include "bmu_can.h"
#include "debug.h"


#define IVT_MONITOR_TASK_PERIOD_MS 1 // not sure what the value should be

void IVTmonitorTask()
{
    if (registerTaskToWatch(IVT_TASK_ID, pdMS_TO_TICKS(IVT_MONITOR_TASK_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register the IVT monitor task with watchdog \n");
        Error_Handler();
    }

    TickType_t LastWakeTick = xTaskGetTickCount(); 
    while(1)
    {
        
        vTaskDelayUntil(LastWakeTick, (IVT_MONITOR_TASK_PERIOD_MS));
        watchdogTaskCheckIn(IVT_TASK_ID);
    }

    
}