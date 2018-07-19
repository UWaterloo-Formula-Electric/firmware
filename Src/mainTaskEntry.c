#include "freertos.h"
#include "task.h"

#include "bsp.h"

void mainTaskFunction(void const * argument)
{
    while (1) {
        HAL_GPIO_TogglePin(DEBUG_LED_PORT, DEBUG_LED_PIN);

        vTaskDelay(1000);
    }
}
