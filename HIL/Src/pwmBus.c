/**
  *****************************************************************************
  * @file    pwmBus.c
  * @brief   Generic PWM output wrappers for the bench PWM channels
  * @details PWM_8, PWM_9 and PWM_10 (PC6/PC7/PC8, TIM8 channels 1-3) are
  * broken out to the bench connectors. Duty cycle is set as a percentage of
  * the timer's live auto-reload value rather than a raw compare count, so
  * callers don't need to know the timer's period. Device specific signal
  * shaping belongs in that device's own file, which should call these rather
  * than the HAL directly.
  *****************************************************************************
  */

#include "pwmBus.h"

#include "bsp.h"
#include "debug.h"

static HAL_StatusTypeDef getPwmTimChannel(PwmChannel_t channel, uint32_t *timChannel)
{
    switch (channel) {
        case PWM_CHANNEL_8:
            *timChannel = PWM_8_CHANNEL;
            return HAL_OK;
        case PWM_CHANNEL_9:
            *timChannel = PWM_9_CHANNEL;
            return HAL_OK;
        case PWM_CHANNEL_10:
            *timChannel = PWM_10_CHANNEL;
            return HAL_OK;
        default:
            return HAL_ERROR;
    }
}

HAL_StatusTypeDef pwmBusInit(void)
{
    uint32_t timChannel;

    for (PwmChannel_t channel = 0; channel < NUM_PWM_CHANNELS; channel++) {
        if (getPwmTimChannel(channel, &timChannel) != HAL_OK) {
            ERROR_PRINT("Failed to init PWM channel %d, bad argument\n", channel);
            return HAL_ERROR;
        }

        if (HAL_TIM_PWM_Start(&PWM_TIM_HANDLE, timChannel) != HAL_OK) {
            ERROR_PRINT("Failed to start PWM channel %d\n", channel);
            return HAL_ERROR;
        }

        if (pwmSetDutyCycle(channel, 0) != HAL_OK) {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef pwmSetDutyCycle(PwmChannel_t channel, float dutyPercent)
{
    uint32_t timChannel;
    uint32_t currentArr;
    uint32_t compare;

    if (dutyPercent < 0.0f || dutyPercent > 100.0f) {
        ERROR_PRINT("Failed to set PWM channel %d, duty cycle %f out of range\n", channel, dutyPercent);
        return HAL_ERROR;
    }

    if (getPwmTimChannel(channel, &timChannel) != HAL_OK) {
        ERROR_PRINT("Failed to set PWM channel %d, bad argument\n", channel);
        return HAL_ERROR;
    }

    currentArr = __HAL_TIM_GET_AUTORELOAD(&PWM_TIM_HANDLE);
    compare = (uint32_t)(dutyPercent * currentArr / 100.0f);

    __HAL_TIM_SET_COMPARE(&PWM_TIM_HANDLE, timChannel, compare);

    return HAL_OK;
}

HAL_StatusTypeDef pwmStop(PwmChannel_t channel)
{
    uint32_t timChannel;

    if (getPwmTimChannel(channel, &timChannel) != HAL_OK) {
        ERROR_PRINT("Failed to stop PWM channel %d, bad argument\n", channel);
        return HAL_ERROR;
    }

    if (HAL_TIM_PWM_Stop(&PWM_TIM_HANDLE, timChannel) != HAL_OK) {
        ERROR_PRINT("Failed to stop PWM channel %d\n", channel);
        return HAL_ERROR;
    }

    return HAL_OK;
}
