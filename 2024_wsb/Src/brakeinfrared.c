#include <stdio.h>
#include <string.h>

#include "cmsis_os.h"
#include "debug.h"
#include "main.h"
#include "bsp.h"
#include "task.h"

#define BRAKE_IR_TASK_PERIOD 1000


void BrakeIRTask(void const* argument) {
    uint16_t raw;
    float voltage;
    float temp;
    HAL_ADC_Start(&MULTISENSOR_ADC_HANDLE);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        HAL_ADC_Start(&MULTISENSOR_ADC_HANDLE);
        HAL_ADC_PollForConversion(&MULTISENSOR_ADC_HANDLE, HAL_MAX_DELAY);
        raw = HAL_ADC_GetValue(&MULTISENSOR_ADC_HANDLE);

        voltage = ((float)raw) / (4095.0f) * 3.3;
        temp = ((voltage / 3) * 450) - 70;

        DEBUG_PRINT("Voltage: %f\r\n", voltage);
        DEBUG_PRINT("Temp: %.2f\r\n", temp);
        vTaskDelayUntil(&xLastWakeTime, BRAKE_IR_TASK_PERIOD);
    }
}