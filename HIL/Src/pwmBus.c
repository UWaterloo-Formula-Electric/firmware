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

<<<<<<< HEAD
=======
// 200 MHz / 500 = 400 kHz counter, so 6.1 Hz (65536 ticks) to 200 Hz (2000 ticks)
// fits the 16 bit counter with at most 0.025% rounding error
#define PWM_FREQ_PRESCALER 500U
#define PWM_FREQ_COUNTER_HZ (PWM_TIM_CLOCK_HZ / PWM_FREQ_PRESCALER)
#define PWM_MIN_PERIOD_TICKS 2U
#define PWM_MAX_PERIOD_TICKS 65536U

>>>>>>> parent of e210a790 (HIL: restore TIM8 defaults when both fan tachs stop, check live channel state)
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
    for (PwmChannel_t channel = 0; channel < NUM_PWM_CHANNELS; channel++) {
        if (pwmInitChannel(channel) != HAL_OK) {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef pwmInitChannel(PwmChannel_t channel)
{
    uint32_t timChannel;

    if (getPwmTimChannel(channel, &timChannel) != HAL_OK) {
        ERROR_PRINT("Failed to init PWM channel %d, bad argument\n", channel);
        return HAL_ERROR;
    }

    // Stop a running channel first so it always comes back from a known state
    if (HAL_TIM_GetChannelState(&PWM_TIM_HANDLE, timChannel) == HAL_TIM_CHANNEL_STATE_BUSY) {
        if (HAL_TIM_PWM_Stop(&PWM_TIM_HANDLE, timChannel) != HAL_OK) {
            ERROR_PRINT("Failed to stop PWM channel %d\n", channel);
            return HAL_ERROR;
        }
    }

    // Channel is now stopped, so this also starts it
    return pwmSetDutyCycle(channel, 0);
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

    // Restart a channel that pwmStop() turned off. Compare is set first so it
    // comes back at the new duty cycle rather than the old one. Skipped when
    // already running, since HAL_TIM_PWM_Start() rejects a busy channel.
    if (HAL_TIM_GetChannelState(&PWM_TIM_HANDLE, timChannel) == HAL_TIM_CHANNEL_STATE_READY) {
        if (HAL_TIM_PWM_Start(&PWM_TIM_HANDLE, timChannel) != HAL_OK) {
            ERROR_PRINT("Failed to start PWM channel %d\n", channel);
            return HAL_ERROR;
        }
    }

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
<<<<<<< HEAD
=======

HAL_StatusTypeDef pwmSetFrequency(PwmChannel_t channel, float frequencyHz, float *actualHz)
{
    uint32_t timChannel;
    float periodTicksFloat;
    uint32_t periodTicks;

    if (getPwmTimChannel(channel, &timChannel) != HAL_OK) {
        ERROR_PRINT("Failed to set PWM channel %d frequency, bad argument\n", channel);
        return HAL_ERROR;
    }

    if (actualHz == NULL || frequencyHz <= 0.0f) {
        ERROR_PRINT("Failed to set PWM channel %d frequency, bad argument\n", channel);
        return HAL_ERROR;
    }

    periodTicksFloat = (float)PWM_FREQ_COUNTER_HZ / frequencyHz + 0.5f;
    if (periodTicksFloat < (float)PWM_MIN_PERIOD_TICKS || periodTicksFloat >= (float)(PWM_MAX_PERIOD_TICKS + 1U)) {
        ERROR_PRINT("Failed to set PWM channel %d, frequency %f out of range\n", channel, frequencyHz);
        return HAL_ERROR;
    }
    periodTicks = (uint32_t)periodTicksFloat;

    __HAL_TIM_SET_PRESCALER(&PWM_TIM_HANDLE, PWM_FREQ_PRESCALER - 1U);
    __HAL_TIM_SET_AUTORELOAD(&PWM_TIM_HANDLE, periodTicks - 1U);
    __HAL_TIM_SET_COMPARE(&PWM_TIM_HANDLE, timChannel, periodTicks / 2U);

    // The prescaler and compare registers are preloaded, so force an update
    // event to load them now. This also restarts the counter at 0, so it never
    // has to run up to 65535 after the auto-reload value shrinks (auto-reload
    // preload is off). Other running channels see one shortened period.
    if (HAL_TIM_GenerateEvent(&PWM_TIM_HANDLE, TIM_EVENTSOURCE_UPDATE) != HAL_OK) {
        ERROR_PRINT("Failed to update PWM timer\n");
        return HAL_ERROR;
    }

    // Same restart rule as pwmSetDutyCycle()
    if (HAL_TIM_GetChannelState(&PWM_TIM_HANDLE, timChannel) == HAL_TIM_CHANNEL_STATE_READY) {
        if (HAL_TIM_PWM_Start(&PWM_TIM_HANDLE, timChannel) != HAL_OK) {
            ERROR_PRINT("Failed to start PWM channel %d\n", channel);
            return HAL_ERROR;
        }
    }

    *actualHz = (float)PWM_FREQ_COUNTER_HZ / (float)periodTicks;
    return HAL_OK;
}
>>>>>>> parent of e210a790 (HIL: restore TIM8 defaults when both fan tachs stop, check live channel state)
