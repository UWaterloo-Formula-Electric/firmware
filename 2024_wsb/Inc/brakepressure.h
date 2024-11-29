#ifndef BRAKE_PRES_H
#define BRAKE_PRES_H
#endif

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

#define BRAKE_PRES_ADC_LOW (409) //at 500mV (the sensor minumum)
#define BRAKE_PRES_ADC_HIGH (3686) //at 4500mV (the sensor maximum)
#define BRAKE_PRES_PSI_LOW (0) //this is a 0-2000PSI sensor
#define BRAKE_PRES_PSI_HIGH (2000)
#define BRAKE_PRES_DEADZONE (100)

#define BRAKE_PRES_TASK_PERIOD 100

void BrakePresTask(void const* argument);
