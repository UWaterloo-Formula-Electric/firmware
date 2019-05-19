#include "bsp.h"
#include "mainTaskEntry.h"
#include "canReceive.h"

#include "freertos.h"
#include "task.h"
#include "debug.h"

#define LED_BLINK_PERIOD_MS 500

void ledTask(void *pvParameters)
{
    while (1)
    {
        if (waitingForHVChange) {
            HAL_GPIO_TogglePin(HV_LED_GPIO_Port, HV_LED_Pin);
        } else {
            if (getHVState()) {
                HV_LED_ON
            } else {
                HV_LED_OFF
            }
        }

        if (waitingForEMChange) {
            HAL_GPIO_TogglePin(EM_LED_GPIO_Port, EM_LED_Pin);
        } else {
            if (getEMState()) {
                EM_LED_ON
            } else {
                EM_LED_OFF
            }
        }

        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_PERIOD_MS));
    }
}
