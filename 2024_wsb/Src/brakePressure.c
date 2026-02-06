#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "bsp.h"
#include "cmsis_os.h"
#include "debug.h"
#include "detectWSB.h"
#include "main.h"
#include "multiSensorADC.h"
#include "task.h"
#include "mathUtils.h"
#if BOARD_ID == ID_WSBFL
#include "wsbfl_can.h"
#endif

#define BRAKE_PRESSURE_TASK_PERIOD 100

/**
 * @brief Get the brake pressure from MSP300 analog sensor. See https://www.mouser.ca/datasheet/2/418/8/ENG_DS_MSP300_B1-1130121.pdf
 * @return Brake pressure in Pascals
 */
int getBrakePressure() {
    // 0.5-4.5V output
    // wsb scales down from 5V to 3.3V
    // we are using the M3033-000005-2K5PG
    float v = get_sensor3_V()/(VOUT_5V * SCALING_5V) * 5;
    float psi = map_range_float(v, 0.5, 4.5, 0, 2500);
    return psi;
}

void BrakePressureTask(void const* argument) {
    deleteWSBTask(WSBFL);
    DEBUG_PRINT("Starting BrakePressureTask\n");
    vTaskDelay(pdMS_TO_TICKS(200)); // Wait for ADC to stabilize
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
#if BOARD_ID == ID_WSBFL
        RearBrakePressure = getBrakePressure();
        sendCAN_WSBFL_BrakePressure();
#endif
        // DEBUG_PRINT("Brake Pressure: %d\n", brakePressure);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(BRAKE_PRESSURE_TASK_PERIOD));
    }
}
