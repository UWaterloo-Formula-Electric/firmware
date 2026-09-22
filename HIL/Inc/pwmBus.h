/**
  *****************************************************************************
  * @file    pwmBus.h
  * @brief   Generic PWM output wrappers for the bench PWM channels
  *****************************************************************************
  */

#ifndef PWM_BUS_H
#define PWM_BUS_H

#include "stm32f7xx_hal.h"

typedef enum {
    PWM_CHANNEL_8 = 0,
    PWM_CHANNEL_9,
    PWM_CHANNEL_10,
    NUM_PWM_CHANNELS
} PwmChannel_t;

// Starts all channels at 0% duty cycle
HAL_StatusTypeDef pwmBusInit(void);

// dutyPercent must be 0-100
HAL_StatusTypeDef pwmSetDutyCycle(PwmChannel_t channel, float dutyPercent);
HAL_StatusTypeDef pwmStop(PwmChannel_t channel);

#endif /* PWM_BUS_H */
