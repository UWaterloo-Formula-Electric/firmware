#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include "stm32f0xx_hal.h"

uint32_t sensor_encoder_count(void);
uint32_t sensor_encoder_mm(void);
float sensor_encoder_speed(void);
HAL_StatusTypeDef sensors_init(void);

#endif
