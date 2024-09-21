#include <stdio.h>
#include <string.h>

#include "cmsis_os.h"
#include "debug.h"
#include "main.h"
#include "bsp.h"
#include "task.h"

#define BRAKE_IR_TASK_PERIOD 1000

double getBrakeTemp(double voltage) {
    // Expects voltage to be between 0V and 3.3
    // Scaling valid for 0V - 3.0V
    // See: https://file.notion.so/f/f/25bbe0bc-a989-4d45-a15e-d6e7c3242dda/f3fbc280-65ef-4afd-8836-2f6c47c351b4/SEN0256-TS01Product_Specification.pdf?table=block&id=5f9b59d9-e026-4e77-abf6-b7f972a7eb6c&spaceId=25bbe0bc-a989-4d45-a15e-d6e7c3242dda&expirationTimestamp=1726704000000&signature=SXDx2oLR74KdCoo4glLvfc6aYe2JPzyO8JGLEZRlrjM&downloadName=%5BSEN0256-TS01%5DProduct+Specification.pdf
    return voltage * 150. - 70.;
}

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
        temp = getBrakeTemp(voltage);

        DEBUG_PRINT("Voltage: %f\r\n", voltage);
        DEBUG_PRINT("Temp: %.2f\r\n", temp);
        vTaskDelayUntil(&xLastWakeTime, BRAKE_IR_TASK_PERIOD);
    }
}