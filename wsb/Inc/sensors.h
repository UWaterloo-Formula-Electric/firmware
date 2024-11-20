#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include "stm32f0xx_hal.h"

////brake pressure new stuff, untested
#if (BOARD_ID == ID_WSBRL)
#define BRAKE_PRES_ADC_LOW (409) //at 500mV (the sensor minumum)
#define BRAKE_PRES_ADC_HIGH (3686) //at 4500mV (the sensor maximum)
#define BRAKE_PRES_PSI_LOW (0) //this is a 0-2000PSI sensor
#define BRAKE_PRES_PSI_HIGH (2000)
#define BRAKE_PRES_DEADZONE (100)
typedef enum ADC_Indices_t {
    BRAKE_PRES_INDEX = 0,
    NUM_ADC_CHANNELS
} ADC_Indices_t;
#define HAS_BRAKE_PRES
#endif
////

uint32_t sensor_encoder_count(void);
uint32_t sensor_encoder_mm(void);
float sensor_encoder_speed(void);
HAL_StatusTypeDef sensors_init(void);

#endif
