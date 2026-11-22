#include "pwmInput.h"
#include "tim.h"

#define PWM_SAMPLE_TIMEOUT_MS 100U

static bool haveFirstRise;
static volatile uint16_t periodTicks;
static volatile uint16_t highTicks;
static volatile uint32_t lastSampleMs;

HAL_StatusTypeDef pwmInputInit(void)
{
    HAL_StatusTypeDef result = HAL_TIM_IC_Start(&htim3, TIM_CHANNEL_2);
    if (result != HAL_OK) {
        return result;
    }

    result = HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
    if (result != HAL_OK) {
        HAL_TIM_IC_Stop(&htim3, TIM_CHANNEL_2);
    }

    return result;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim != &htim3 || htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1) {
        return;
    }

    bool haveFall = __HAL_TIM_GET_FLAG(htim, TIM_FLAG_CC2) != RESET;
    uint16_t period = (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
    uint16_t high = (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

    const uint32_t overcaptureFlags = TIM_FLAG_CC1OF | TIM_FLAG_CC2OF;
    bool overcapture = (htim->Instance->SR & overcaptureFlags) != 0U;

    if (overcapture) {
        __HAL_TIM_CLEAR_FLAG(htim, overcaptureFlags);
    }

    if (haveFirstRise && haveFall && !overcapture && high > 0U && high < period) {
        periodTicks = period;
        highTicks = high;
        lastSampleMs = HAL_GetTick();
    }

    haveFirstRise = true;
}

bool pwmInputRead(uint16_t *period, uint16_t *high)
{
    uint32_t savedPrimask = __get_PRIMASK();
    __disable_irq();

    uint16_t capturedPeriod = periodTicks;
    uint16_t capturedHigh = highTicks;
    uint32_t capturedAt = lastSampleMs;

    __set_PRIMASK(savedPrimask);

    if (capturedPeriod == 0U ||
        (HAL_GetTick() - capturedAt) >= PWM_SAMPLE_TIMEOUT_MS) {
        return false;
    }

    *period = capturedPeriod;
    *high = capturedHigh;
    return true;
}