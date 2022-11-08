#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"

#define MAIN_TASK_PERIOD 1000

void mainTaskFunction(void const * argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    DEBUG_PRINT("Starting up!!\n");
    while (1) {
        HAL_GPIO_TogglePin(DEBUG_LED_PORT, DEBUG_LED_PIN);

        vTaskDelayUntil(&xLastWakeTime, MAIN_TASK_PERIOD);
    }
}
