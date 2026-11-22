#include "pwmInput.h"
#include "bsp.h"
#include "debug.h"

HAL_StatusTypeDef pwmInputInit(void)
{
    HAL_StatusTypeDef result = HAL_TIM_IC_Start(&PWM_INPUT_TIM_HANDLE, PWM_INPUT_HIGH_CHANNEL);

    if (result != HAL_OK) {
        ERROR_PRINT("Failed to start PWM high-time capture\n");
        return result;
    }

    result = HAL_TIM_IC_Start(&PWM_INPUT_TIM_HANDLE, PWM_INPUT_PERIOD_CHANNEL);

    if (result != HAL_OK) {
        ERROR_PRINT("Failed to start PWM period capture\n");
        if (HAL_TIM_IC_Stop(&PWM_INPUT_TIM_HANDLE, PWM_INPUT_HIGH_CHANNEL) != HAL_OK) {
            ERROR_PRINT("Failed to stop PWM high-time capture\n");
        }
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
    __HAL_TIM_CLEAR_FLAG(&PWM_INPUT_TIM_HANDLE, captureFlags);

    while ((HAL_GetTick() - startedAt) < PWM_READ_TIMEOUT_MS) {
        if (__HAL_TIM_GET_FLAG(&PWM_INPUT_TIM_HANDLE, TIM_FLAG_CC1) == RESET) {
            continue;
        }

        // Discard the first rising edge
        if (!haveFirstRise) {
            (void)HAL_TIM_ReadCapturedValue(&PWM_INPUT_TIM_HANDLE, PWM_INPUT_PERIOD_CHANNEL);
            __HAL_TIM_CLEAR_FLAG(&PWM_INPUT_TIM_HANDLE, captureFlags);
            haveFirstRise = true;
            continue;
        }

        uint32_t statusBefore = PWM_INPUT_TIM_HANDLE.Instance->SR;
        uint16_t capturedPeriod = (uint16_t)HAL_TIM_ReadCapturedValue(&PWM_INPUT_TIM_HANDLE, PWM_INPUT_PERIOD_CHANNEL);
        uint16_t capturedHigh = (uint16_t)HAL_TIM_ReadCapturedValue(&PWM_INPUT_TIM_HANDLE, PWM_INPUT_HIGH_CHANNEL);
        uint32_t statusAfter = PWM_INPUT_TIM_HANDLE.Instance->SR;

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
        __HAL_TIM_CLEAR_FLAG(&PWM_INPUT_TIM_HANDLE, captureFlags);
        haveFirstRise = false;
    }

    return false;
}

uint32_t pwmInputCounterClockHz(void)
{
    uint32_t pclk = HAL_RCC_GetPCLK1Freq();
    uint32_t hclk = HAL_RCC_GetHCLKFreq();
    uint32_t timerClock;

    if ((RCC->DCKCFGR1 & RCC_DCKCFGR1_TIMPRE) == 0U) {
        if ((RCC->CFGR & RCC_CFGR_PPRE1) == 0U) {
            timerClock = pclk;
        } else {
            timerClock = 2U * pclk;
        }
    } else {
        if ((hclk / pclk) <= 4U) {
            timerClock = hclk;
        } else {
            timerClock = 4U * pclk;
        }
    }

    return timerClock / (PWM_INPUT_TIM_HANDLE.Instance->PSC + 1U);
}