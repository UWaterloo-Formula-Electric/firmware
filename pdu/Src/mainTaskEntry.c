#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "uwfe_debug.h"

#define MAIN_TASK_PERIOD 1000

void mainTaskFunction(void const * argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    printf("Starting up!!\n");
    while (1) {
        /*printf("Hello\n");*/
        HAL_GPIO_TogglePin(DEBUG_LED_PORT, DEBUG_LED_PIN);

        vTaskDelayUntil(&xLastWakeTime, MAIN_TASK_PERIOD);
    }
}
