#include "FreeRTOS.h"
#include "bsp.h"
#include "debug.h"
#include "drive_by_wire.h"
#include "stm32f7xx_hal.h"
#include "task.h"

// Useful links https://www.ganssle.com/debouncing-pt2.htm https://github.com/tcleg/Button_Debouncer/blob/master/C/button_debounce.c
#define NUM_BUTTON_STATES 4  // Holds the states that the particular port is transitioning through

#define BUTTON_TASK_PERIOD_MS 5


/**
 * @brief Continuously monitor HV, EM, TC, Endurance buttons and debounce them.
 */
void buttonTask(void *pvParameters) {
    uint8_t index = 0;
    uint8_t state[NUM_BUTTON_STATES];
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(BUTTON_TASK_PERIOD_MS));
    }
}