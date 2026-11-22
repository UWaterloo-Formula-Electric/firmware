#include "pwmInput.h"
#include "tim.h"

#define PWM_READ_TIMEOUT_MS 10U

HAL_StatusTypeDef pwmInputInit(void)
{
    HAL_StatusTypeDef result = HAL_TIM_IC_Start(&htim3, TIM_CHANNEL_2);

    if (result != HAL_OK) {
        return result;
    }

    result = HAL_TIM_IC_Start(&htim3, TIM_CHANNEL_1);

    if (result != HAL_OK) {
        HAL_TIM_IC_Stop(&htim3, TIM_CHANNEL_2);
    }

    return result;
}

bool pwmInputRead(uint16_t *period, uint16_t *high)
{
    if (period == NULL || high == NULL) {
        return false;
    }

    const uint32_t captureFlags = TIM_FLAG_CC1 | TIM_FLAG_CC2 | TIM_FLAG_CC1OF | TIM_FLAG_CC2OF;

    const uint32_t overcaptureFlags = TIM_FLAG_CC1OF | TIM_FLAG_CC2OF;

    uint32_t startedAt = HAL_GetTick();
    bool haveFirstRise = false;

    // Discard captures left over from before this read request
    __HAL_TIM_CLEAR_FLAG(&htim3, captureFlags);

    while ((HAL_GetTick() - startedAt) < PWM_READ_TIMEOUT_MS) {
        if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_CC1) == RESET) {
            continue;
        }

        // Discard the first rising edge
        if (!haveFirstRise) {
            (void)HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_1);
            __HAL_TIM_CLEAR_FLAG(&htim3, captureFlags);
            haveFirstRise = true;
            continue;
        }

        uint32_t statusBefore = htim3.Instance->SR;
        uint16_t capturedPeriod = (uint16_t)HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_1);
        uint16_t capturedHigh = (uint16_t)HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_2);
        uint32_t statusAfter = htim3.Instance->SR;

        bool haveFall = (statusBefore & TIM_FLAG_CC2) != 0U;
        bool overwritten = ((statusBefore | statusAfter) & overcaptureFlags) != 0U;

        // Reading the capture registers clears their capture flags
        bool newCapture = (statusAfter & (TIM_FLAG_CC1 | TIM_FLAG_CC2)) != 0U;

        if (haveFall && !overwritten && !newCapture &&
            capturedHigh > 0U && capturedHigh < capturedPeriod) {
            *period = capturedPeriod;
            *high = capturedHigh;
            return true;
        }

        // Retry with a new complete cycle
        __HAL_TIM_CLEAR_FLAG(&htim3, captureFlags);
        haveFirstRise = false;
    }

    return false;
}