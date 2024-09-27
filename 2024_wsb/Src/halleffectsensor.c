#include <stdio.h>
#include <string.h>

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

float getWheelSpeed() {
    /* hw timer set in hall effect mode, increments each time a gear passes by
     * reads count diff & time diff
     * teeth/s = count diff/time diff
     * rps = 1/num_teeth * teeth/s
     * rpm = 60rps
     * 2024 car: 17 teeth measured
     */
    return 0;
}

void StartHallEffectSensorTask(void const * argument) {
    DEBUG_PRINT("Starting StartHallEffectSensorTask\n");
    WSBType_t wsbType = detectWSB();
    if (wsbType != WSBRL || wsbType != WSBRR) {
        DEBUG_PRINT("Invalid wsb: not WSBFL or WSBRR, deleting BrakeIRTask\n");
        vTaskDelete(NULL);
        return;
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {

    }
}