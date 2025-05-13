#ifndef BRAKE_PRES_C
#define BRAKE_PRES_C
#endif

#include <stdbool.h>
#include <stddef.h>
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

#define BRAKE_PRES_ADC_LOW (409) //at 500mV (the sensor minumum)
#define BRAKE_PRES_ADC_HIGH (3686) //at 4500mV (the sensor maximum)
#define BRAKE_PRES_PSI_LOW (0) //this is a 0-2000PSI sensor
#define BRAKE_PRES_PSI_HIGH (2000)
//#define BRAKE_PRES_DEADZONE (100)

#define BRAKE_PRES_TASK_PERIOD 100

#if BOARD_ID == ID_WSBFR
#include "wsbfr_can.h"
#endif

int32_t map_range(int32_t in, int32_t low, int32_t high, int32_t low_out, int32_t high_out) { //copied from brake and throttle code in the VCU
    if (in < low) {
        in = low;
    } else if (in > high) {
        in = high;
    }
    int in_range = high - low;
    int out_range = high_out - low_out;

    return (in - low) * out_range / in_range + low_out;
}

uint32_t calcPres(uint32_t raw) {
	// note: the pressure reading will clip if the sensor voltage reading is out of range
	return map_range(raw, BRAKE_PRES_ADC_LOW, BRAKE_PRES_ADC_HIGH, BRAKE_PRES_PSI_LOW, BRAKE_PRES_ADC_HIGH);
}

bool isPresInRange(uint32_t raw) {
	//we account for if it's off by a little on the ends, so if it's slightly lower or higher it just clips to 0 or 2000
    //return BRAKE_PRES_ADC_LOW-BRAKE_PRES_DEADZONE <= raw && raw <= BRAKE_PRES_ADC_HIGH+BRAKE_PRES_DEADZONE;
	return BRAKE_PRES_ADC_LOW <= raw && raw <= BRAKE_PRES_ADC_HIGH;
}
void BrakePresTask(void const* argument) {
    deleteWSBTask(WSBFR);
    DEBUG_PRINT("Starting BrakePresTask\n");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {

#if BOARD_ID == ID_WSBFR
    	uint32_t raw = get_sensor3_V(); //sensor 3 it is
        BrakePresRear = calcPres(raw);
        BrakePresRearValid = isPresInRange(raw);
        sendCAN_WSBFR_BrakePres();
#endif

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(BRAKE_PRES_TASK_PERIOD));
    }
}
