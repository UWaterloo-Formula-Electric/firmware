#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stdbool.h"

#include "bsp.h"
#include "debug.h"
#include "userCan.h"
#include "watchdog.h"

#define MAIN_TASK_ID 1
#define MAIN_TASK_PERIOD_MS 1000

void mainTaskFunction(void const * argument)
{
    if (registerTaskToWatch(MAIN_TASK_ID, 5*pdMS_TO_TICKS(MAIN_TASK_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register main task with watchdog!\n");
        Error_Handler();
    }
    
    DEBUG_PRINT("Started Up");

    while (1) {
        HAL_GPIO_TogglePin(DEBUG_LED_PORT, DEBUG_LED_PIN);

        watchdogTaskCheckIn(MAIN_TASK_ID);
        vTaskDelay(pdMS_TO_TICKS(MAIN_TASK_PERIOD_MS));
    }
}
