#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "FreeRTOS.h"
#include "bsp.h"
#include "cmsis_os.h"
#include "debug.h"
#include "detectWSB.h"
#include "main.h"
#include "multiSensorADC.h"
#include "task.h"
#elif BOARD_ID == ID_WSBRL
#include "wsbrl_can.h"
#endif

int map_range(int in, int low, int high, int low_out, int high_out) { //copied from brake and throttle code in the VCU
    if (in < low) {
        in = low;
    } else if (in > high) {
        in = high;
    }
    int in_range = high - low;
    int out_range = high_out - low_out;

    return (in - low) * out_range / in_range + low_out;
}

int calcPres(uint32_t raw) {
	// note: the pressure reading will clip if the sensor voltage reading is out of range
	return map_range(raw, BRAKE_PRES_ADC_LOW, BRAKE_PRES_ADC_HIGH, BRAKE_PRES_PSI_LOW, BRAKE_PRES_ADC_HIGH);
}

bool isPresInRange(uint32_t raw) {
	//we account for if it's off by a little on the ends, so if it's slightly lower or higher it just clips to 0 or 2000
    return BRAKE_PRES_ADC_LOW-BRAKE_PRES_DEADZONE <= raw && raw <= BRAKE_PRES_ADC_HIGH+BRAKE_PRES_DEADZONE
}
void BrakePresTask(void const* argument) {
    deleteWSBTask(WSBRL);
    DEBUG_PRINT("Starting BrakePresTask\n");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {

#elif BOARD_ID == ID_WSBRL
    	uint32_t raw = get_sensor1_V(); //still don't know which ADC channel it is, hope 1 is correct
        BrakePresRear = calcPres(raw);
        BrakePresRearValid = isPresInRange(raw);
        sendCAN_WSBRL_BrakePres();
#endif

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(BRAKE_PRES_TASK_PERIOD));
    }
}
