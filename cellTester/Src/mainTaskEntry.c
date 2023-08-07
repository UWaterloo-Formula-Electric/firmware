/**
 *****************************************************************************
 * @file    mainTaskEntry.c
 * @author  Justin Vuong
 * @brief   Module containing main task, which is the default task for all
 * boards. It currently blinks the debug LED to indicate the firmware is running
 *****************************************************************************
 */

#include <stdbool.h>

#include "FreeRTOS.h"
#include "bsp.h"
#include "debug.h"
#include "fetControl.h"
#include "mock.h"
#include "task.h"
#include "uartRXTask.h"

#define MAIN_TASK_PERIOD 1000
#define CELL_STABILIZATION_TIME_MS 10

void printCellValues();

void mainTaskFunction(void const* argument) {    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    DEBUG_PRINT("Starting up!!\n");
    // Charecterization process:
    // 1. Start new characterization
    // 2. Increment cell current by changing pwm duty cycle
    // 3. Wait for cell to stabilize
    // 4. Take measurement
    // 5. Repeat 2-4 until cell current is at max
    while (1) {
        if (isCharacterizationRunning) {
            for (float dutyCycle = 0; dutyCycle < 100; dutyCycle += 0.5) {
                set_PWM_Duty_Cycle(&FET_TIM_HANDLE, dutyCycle);
                vTaskDelay(pdMS_TO_TICKS(CELL_STABILIZATION_TIME_MS));
                printCellValues();
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            isCharacterizationRunning = false;
            set_PWM_Duty_Cycle(&FET_TIM_HANDLE, 0);
        }
        printCellValues();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(MAIN_TASK_PERIOD));
    }
}

void printCellValues() {
    // Timestamp, Charecterization Enabled, Voltage, Current, Temperature
    DEBUG_PRINT("%lu, %u, %.3lf, %.3lf, %.2lf",
                HAL_GetTick(),
                isCharacterizationRunning,
                get_cell_voltage(),
                get_cell_current(),
                get_cell_temperature());
}