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
#include "ade7913.h"
#include "temperature.h"

#define MAIN_TASK_PERIOD 1000
#define CELL_STABILIZATION_TIME_MS 10

void printCellValues(void);
void printThermistorValues(void);

void mainTaskFunction(void const* argument) {
    // Wait for boot
    vTaskDelay(pdMS_TO_TICKS(100));
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // DEBUG_PRINT("Starting up!!\n");

    if (hvadc_init() != HAL_OK)
    {
        DEBUG_PRINT("HVADC init fail\n");
        Error_Handler();
    }
    
    // Configure ADCs    
    if (thermistor_adc_init(cell_i2c_hdr) != HAL_OK)
    {
        DEBUG_PRINT("cell ADC init fail\n");
        Error_Handler();
    }
    
    if (thermistor_adc_init(fuse_i2c_hdr) != HAL_OK)
    {
        DEBUG_PRINT("fuse ADC init fail\n");
        Error_Handler();
    }
    
    // Charecterization process:
    // 1. Start new characterization
    // 2. Increment cell current by changing pwm duty cycle
    // 3. Wait for cell to stabilize
    // 4. Take measurement
    // 5. Repeat 2-4 until cell current is at max
    set_PWM_Duty_Cycle(&FET_TIM_HANDLE, 0);
    DEBUG_PRINT("time (ms), isCharacterising, PWM duty (%%), v1 (V), v2 (V), Ishunt (A), Temp1 (C), Temp2 (C)\n");

    while (1) {
        if (isCharacterizationRunning) {
            for (float dutyCycle = 0; dutyCycle <= 100; dutyCycle += 0.5) {
                if (!isCharacterizationRunning)
                    break;
                set_PWM_Duty_Cycle(&FET_TIM_HANDLE, dutyCycle);
                vTaskDelay(pdMS_TO_TICKS(CELL_STABILIZATION_TIME_MS));
                printCellValues();
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            isCharacterizationRunning = false;
            set_PWM_Duty_Cycle(&FET_TIM_HANDLE, 0);
        }
        printCellValues();
        printThermistorValues();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(MAIN_TASK_PERIOD));
    }
}

void printCellValues(void) {
    float v1 = 0.0f;
    float v2 = 0.0f;
    float I = 0.0f;
    adc_read_v1(&v1);
    adc_read_v2(&v2);
    adc_read_current(&I);
    // Timestamp, Charecterization Enabled, Voltage, Current, Temperature
    DEBUG_PRINT("%lu, %u, %.3lf, %.3lf, %.3lf, %.2lf, ",
                HAL_GetTick()/1000,
                isCharacterizationRunning,
                get_PWM_Duty_Cycle(),
                v1,
                I,
                v2);
}

void printThermistorValues(void)
{
    float cell_temp_result = 0.0f;
    float fuse_temp_result = 0.0f;

    if (read_thermistor(cell_i2c_hdr, &cell_temp_result) != HAL_OK) {
        DEBUG_PRINT("failed to read cell temp\n");
    }
    DEBUG_PRINT("%f, ", cell_temp_result);
    
    if (read_thermistor(fuse_i2c_hdr, &fuse_temp_result) != HAL_OK) {
        DEBUG_PRINT("failed to read fuse temp\n");
    }
    DEBUG_PRINT("%f\n", fuse_temp_result);
}