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
#include "task.h"
#include "uartRXTask.h"
#include "ade7913.h"
#include "temperature.h"

#define MAIN_TASK_PERIOD 3
#define CELL_STABILIZATION_TIME_MS 10

// Hardware defined constant
#define CELL_TESTER_MIN_CURRENT_A 1.5f

void updateFetDuty(float lastCurrentMeasurement);
void getCellValues(float* current);
void printThermistorValues(void);

// fetDutyCycle = [0, 100]
static float fetDutyCycle = 0.0f;
static float kI_cellTester = 0.25f;

void mainTaskFunction(void const* argument) {
    // Wait for boot
    vTaskDelay(pdMS_TO_TICKS(100));
    TickType_t xLastWakeTime = xTaskGetTickCount();
    float current = 0.0f;

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
    set_PWM_Duty_Cycle(&FET_TIM_HANDLE, 0.0f);
    DEBUG_PRINT("time (ms), isCharacterising, PWM duty (%%), v1 (V), v2 (V), Ishunt (A), Temp1 (C), Temp2 (C)\n");
    while (1) {
        getCellValues(&current);
        // printThermistorValues();

        updateFetDuty(current);
        set_PWM_Duty_Cycle(&FET_TIM_HANDLE, fetDutyCycle);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(MAIN_TASK_PERIOD));
    }
}

void updateFetDuty(float lastCurrentMeasurement)
{
    if (getCurrentTarget() <= CELL_TESTER_MIN_CURRENT_A)
    {
        fetDutyCycle = 0.0f;
        return;
    }

    const float current_error = getCurrentTarget() - lastCurrentMeasurement;
    fetDutyCycle += current_error * kI_cellTester;

    if (fetDutyCycle < 0)
    {
        fetDutyCycle = 0;
    }
    else if (fetDutyCycle > 100)
    {
        fetDutyCycle = 100;
    }
}

void getCellValues(float* current) {
    float fuse_temp_result = 0.0f;
    // // float fuse_temp_result = 0.0f;

    if (read_thermistor(fuse_i2c_hdr, &fuse_temp_result) != HAL_OK) {
        DEBUG_PRINT("failed to read cell temp\n");
    }

    float v = 0.0f;
    adc_read_v(&v);
    adc_read_current(current);
    // Timestamp, Voltage, Current, Temperature
    DEBUG_PRINT("%lu, %.3lf, %.3lf, %.2lf\n",
                HAL_GetTick(),
                v,
                *current,
                fuse_temp_result);
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
