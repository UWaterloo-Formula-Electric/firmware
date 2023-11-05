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

#define MAIN_TASK_PERIOD_MS 4
#define CELL_STABILIZATION_TIME_MS 10

// Hardware defined constant
#define CELL_TESTER_MIN_CURRENT_A 1.5f
#define CELL_TEST_CURRENT_A 40.0f

#define FET_CONTROL_KP 0.015f

void updateCurrentTarget(void);
void updateFetDuty(float lastCurrentMeasurement);
void updateCellValues(float* current, float* v1, float* v2);

// fetDutyCycle = [0, 100]
static float fetDutyCycle = PWM_MAX_DUTY_NO_CURRENT;

void mainTaskFunction(void const* argument) {
    // Wait for boot
    vTaskDelay(pdMS_TO_TICKS(100));
    TickType_t xLastWakeTime = xTaskGetTickCount();
    float current = 0.0f;
    float hv_adc_v1 = 0.0f;
    float hv_adc_v2 = 0.0f;

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
    DEBUG_PRINT("Time (ms), IShunt (A), V1 (V), V2 (V), T1 (C), T2 (C)\n");
    while (1) {
        updateCellValues(&current, &hv_adc_v1, &hv_adc_v2);
        // printThermistorValues();

        updateCurrentTarget();
        updateFetDuty(current);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(MAIN_TASK_PERIOD_MS));
    }
}

void updateCurrentTarget(void)
{
    switch (getCellTestStatus())
    {
        case CellTestStatus_RUNNING:
            setCurrentTarget(CELL_TEST_CURRENT_A);
            break;

        case CellTestStatus_LOGGING:
            setCurrentTarget(0.0f);
            break;
        
        default:
            // Lint
            break;
    }
}

void updateFetDuty(float lastCurrentMeasurement)
{
    const float current_error = getCurrentTarget() - lastCurrentMeasurement;
    fetDutyCycle += current_error * FET_CONTROL_KP;

    if (fetDutyCycle < 0)
    {
        fetDutyCycle = 0;
    }
    else if (fetDutyCycle > 100)
    {
        fetDutyCycle = 100;
    }

    set_PWM_Duty_Cycle(&FET_TIM_HANDLE, fetDutyCycle);
}

void updateCellValues(float* current, float* v1, float* v2) {

    float cell_temp_result = 0.0f;
    float fuse_temp_result = 0.0f;

    if (read_thermistor(cell_i2c_hdr, &cell_temp_result) != HAL_OK) {
        DEBUG_PRINT("failed to read cell temp\n");
    }
    if (read_thermistor(fuse_i2c_hdr, &fuse_temp_result) != HAL_OK) {
        // DEBUG_PRINT("failed to read fuse temp\n");
    }
    adc_read_v1(v1);
    adc_read_v2(v2);
    adc_read_current(current);
    // Timestamp, Charecterization Enabled, Voltage, Current, Temperature
    DEBUG_PRINT("%lu, %.3lf, %.3lf, %.3lf, %.3lf, %.3lf\r\n",
                HAL_GetTick(),
                *current,
                *v1,
                *v2,
                cell_temp_result,
                fuse_temp_result);
}

// Set duty cycle to lowest PWM duty that draws 0 current to improve response time of controller
void resetFetDutyCycle(void)
{
    fetDutyCycle = PWM_MAX_DUTY_NO_CURRENT;
}