#include <stdio.h>
#include <string.h>

#include "cmsis_os.h"
#include "debug.h"
#include "main.h"
#include "bsp.h"
#include "multiSensorADC.h"
#include "task.h"
#include "wsbfl_can.h"

#define BRAKE_IR_TASK_PERIOD 100
#define BRAKE_IR_OUT_OF_RANGE_ACC 7

float getBrakeTemp() {
    // Expects voltage to be between 0V and 3.3
    // Scaling valid for 0V - 3.0V
    // See: https://file.notion.so/f/f/25bbe0bc-a989-4d45-a15e-d6e7c3242dda/f3fbc280-65ef-4afd-8836-2f6c47c351b4/SEN0256-TS01Product_Specification.pdf?table=block&id=5f9b59d9-e026-4e77-abf6-b7f972a7eb6c&spaceId=25bbe0bc-a989-4d45-a15e-d6e7c3242dda&expirationTimestamp=1726704000000&signature=SXDx2oLR74KdCoo4glLvfc6aYe2JPzyO8JGLEZRlrjM&downloadName=%5BSEN0256-TS01%5DProduct+Specification.pdf
    return getBrakeIrVoltage() * 150. - 70.;
}


uint8_t getBrakeTempAccuracy(float brakeTemp){
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
    DEBUG_PRINT("Starting BrakeIRTask\n");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        BrakeTemp = getBrakeTemp();
        BrakeTempAccuracy = getBrakeTempAccuracy(BrakeTemp);
        sendCAN_WSBFL_BrakeTemp();
        DEBUG_PRINT("Temp: %.2f\r\n", getBrakeTemp());
        vTaskDelayUntil(&xLastWakeTime, BRAKE_IR_TASK_PERIOD);
    }
}