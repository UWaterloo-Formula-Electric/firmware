#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"

void mainTaskFunction(void const * argument)
{
    printf("Starting up!!\n");
    while (1) {
        /*printf("Hello\n");*/
        HAL_GPIO_TogglePin(DEBUG_LED_PORT, DEBUG_LED_PIN);

        vTaskDelay(1000);
    }
}
