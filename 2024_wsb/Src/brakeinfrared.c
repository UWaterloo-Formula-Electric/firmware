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
#if BOARD_ID == ID_WSBFL
#include "wsbfl_can.h"
#elif BOARD_ID == ID_WSBRR
#include "wsbrr_can.h"
#endif
#define BRAKE_IR_TASK_PERIOD 100
#define BRAKE_IR_OUT_OF_RANGE_ACC 7

float getBrakeTemp() {
    // Expects voltage to be between 0V and 3.3
    // Scaling valid for 0V - 3.0V
    // See: https://file.notion.so/f/f/25bbe0bc-a989-4d45-a15e-d6e7c3242dda/f3fbc280-65ef-4afd-8836-2f6c47c351b4/SEN0256-TS01Product_Specification.pdf?table=block&id=5f9b59d9-e026-4e77-abf6-b7f972a7eb6c&spaceId=25bbe0bc-a989-4d45-a15e-d6e7c3242dda&expirationTimestamp=1726704000000&signature=SXDx2oLR74KdCoo4glLvfc6aYe2JPzyO8JGLEZRlrjM&downloadName=%5BSEN0256-TS01%5DProduct+Specification.pdf
    // Apparently its U/3*340-70 https://wiki.dfrobot.com/TS01_IR_Thermal_Sensor_(0-3V)_SKU_SEN0256
    return get_sensor3_V() / 3 * 340 - 70;
}

uint8_t getBrakeTempAccuracy(float brakeTemp) {
    // See: https://file.notion.so/f/f/25bbe0bc-a989-4d45-a15e-d6e7c3242dda/f3fbc280-65ef-4afd-8836-2f6c47c351b4/SEN0256-TS01Product_Specification.pdf?table=block&id=5f9b59d9-e026-4e77-abf6-b7f972a7eb6c&spaceId=25bbe0bc-a989-4d45-a15e-d6e7c3242dda&expirationTimestamp=1726704000000&signature=SXDx2oLR74KdCoo4glLvfc6aYe2JPzyO8JGLEZRlrjM&downloadName=%5BSEN0256-TS01%5DProduct+Specification.pdf
    // Assume the temperature of the IR sensor itself (not what it is measuring) is between 10C - 45C (see page 4)
    if (brakeTemp < -70 || brakeTemp > 380)
        return BRAKE_IR_OUT_OF_RANGE_ACC;

    if (brakeTemp < -40)
        return 2;
    else if (brakeTemp < 0)
        return 1;
    else if (brakeTemp < 60)
        return 0;
    else if (brakeTemp < 120)
        return 1;
    else if (brakeTemp < 180)
        return 2;
    else if (brakeTemp < 240)
        return 3;
    else
        return 4;
}

void BrakeIRTask(void const* argument) {
    deleteWSBTask(WSBFL | WSBRR);
    DEBUG_PRINT("Starting BrakeIRTask\n");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
#if BOARD_ID == ID_WSBFL || BOARD_ID == ID_WSBRR
        float brakeTemp = getBrakeTemp();
        uint8_t brakeTempAccuracy = getBrakeTempAccuracy(brakeTemp);
#endif
#if BOARD_ID == ID_WSBFL
        BrakeTempFront = brakeTemp;
        BrakeTempFrontAccuracy = brakeTempAccuracy;
        sendCAN_WSBFL_BrakeTemp();
#elif BOARD_ID == ID_WSBRR
        BrakeTempRear = brakeTemp;
        BrakeTempRearAccuracy = brakeTempAccuracy;
        sendCAN_WSBRR_BrakeTemp();
#endif

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(BRAKE_IR_TASK_PERIOD));
    }
}