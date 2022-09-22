#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"

void mainTaskFunction(void const * argument)
{
    DEBUG_PRINT("Starting up!!\n");
    while (1) {
        HAL_GPIO_TogglePin(DEBUG_LED_PORT, DEBUG_LED_PIN);
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_2);
        vTaskDelay(1000);
    }
}
