#include <string.h>

#include "FreeRTOS.h"
#include "bsp.h"
#include "debug.h"
#include "drive_by_wire.h"
#include "state_machine.h"
#include "stm32f7xx_hal.h"
#include "task.h"

// Useful links https://www.ganssle.com/debouncing-pt2.htm https://github.com/tcleg/Button_Debouncer/blob/master/C/button_debounce.c
#define DEBOUNCE_TIME_MS 50
#define BUTTON_TASK_PERIOD_MS 5
#define NUM_DEBOUNCE_CHECKS (DEBOUNCE_TIME_MS / BUTTON_TASK_PERIOD_MS)  // Holds the states that the particular port is transitioning through

enum ButtonOffsets {
    HV_TOGGLE_OFFSET = 0,
    EM_TOGGLE_OFFSET,
    TC_TOGGLE_OFFSET,
    ENDURANCE_TOGGLE_OFFSET
};


void buttonTask(void *pvParameters) {
    uint8_t index = 0;
    uint8_t state[NUM_DEBOUNCE_CHECKS];
    memset(state, 0, sizeof(state));

    uint8_t lastDebouncedState = 0;
    uint8_t debouncedState = 0;

    while (1) {
        lastDebouncedState = debouncedState;
        // Read the state of the buttons
        state[index] = 0;
        state[index] |= (HAL_GPIO_ReadPin(HV_TOGGLE_BUTTON_PORT, HV_TOGGLE_BUTTON_PIN) == GPIO_PIN_RESET) << HV_TOGGLE_OFFSET;
        state[index] |= (HAL_GPIO_ReadPin(EM_TOGGLE_BUTTON_PORT, EM_TOGGLE_BUTTON_PIN) == GPIO_PIN_RESET) << EM_TOGGLE_OFFSET;
        state[index] |= (HAL_GPIO_ReadPin(TC_TOGGLE_BUTTON_PORT, TC_TOGGLE_BUTTON_PIN) == GPIO_PIN_RESET) << TC_TOGGLE_OFFSET;
        state[index] |= (HAL_GPIO_ReadPin(ENDURANCE_TOGGLE_BUTTON_PORT, ENDURANCE_TOGGLE_BUTTON_PIN) == GPIO_PIN_RESET) << ENDURANCE_TOGGLE_OFFSET;

        // Debounce the buttons
        debouncedState = 0xFF;  // Start with all bits set
        for (int i = 0; i < NUM_DEBOUNCE_CHECKS; i++) {
            // DEBUG_PRINT("state[%d]: %d\n", i, state[i]);
            debouncedState &= state[i];
        }

        // DEBUG_PRINT("raw: %d, deb: %d\n", state[index], debouncedState);
        index = (index + 1) % NUM_DEBOUNCE_CHECKS;

        uint8_t changed = debouncedState ^ lastDebouncedState;
        // DEBUG_PRINT("changed: %d, last: %d, debounced: %d\n", changed, lastDebouncedState, debouncedState);
        switch (changed & debouncedState) {
            case 1u << HV_TOGGLE_OFFSET:
                DEBUG_PRINT("received HV button\n");
                fsmSendEvent(&VCUFsmHandle, EV_BTN_HV_Toggle, portMAX_DELAY);
                break;
            case 1u << EM_TOGGLE_OFFSET:
                DEBUG_PRINT("received EM button\n");
                fsmSendEvent(&VCUFsmHandle, EV_BTN_EM_Toggle, portMAX_DELAY);
                break;
            case 1u << TC_TOGGLE_OFFSET:
                DEBUG_PRINT("received TC button\n");
                fsmSendEvent(&VCUFsmHandle, EV_BTN_TC_Toggle, portMAX_DELAY);
                break;
            case 1u << ENDURANCE_TOGGLE_OFFSET:
                DEBUG_PRINT("received endurance button\n");
                fsmSendEvent(&VCUFsmHandle, EV_BTN_Endurance_Mode_Toggle, portMAX_DELAY);
                break;
            case 0:
                // No button pressed
                break;
            default:
                // invalid number of buttons pressed
                ERROR_PRINT("Unknown button pressed\n");
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(BUTTON_TASK_PERIOD_MS));
    }
}