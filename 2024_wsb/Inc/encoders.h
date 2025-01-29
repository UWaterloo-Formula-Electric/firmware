#ifndef __ENOCDERS_H__
#define __ENOCDERS_H__
#include "bsp.h"
#include "stm32f4xx_hal.h"

#define SAMPLES_PER_SEC (1000 / ENCODER_RPS_TIM_MS)

// Front wheels
#define QUAD_ENCODER 4
#define TICKS_PER_QUAD_ENC_REV (2048 * QUAD_ENCODER)

// Rear wheels
#define NUM_REAR_SPOKE_TEETH 16

extern volatile float frontRPS;
extern volatile float rearRPS;
HAL_StatusTypeDef encoders_init(void);

#endif  // __ENOCDERS_H__