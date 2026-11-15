#include "pwmInput.h"
#include "bsp.h"

#include <stdbool.h>
#include <stdint.h>

static uint16_t lastRise;
static uint16_t pendingHighTicks;
static bool haveRise;
static bool haveFall;

// Most recent complete PWM cycle. Zero period means no sample yet.
static volatile uint16_t periodTicks;
static volatile uint16_t highTicks;
static volatile uint32_t lastSampleMs;

HAL_StatusTypeDef pwmInputInit(void)
{
    return HAL_TIM_IC_Start_IT(&PWM_TIM_HANDLE, TIM_CHANNEL_1);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim != &PWM_TIM_HANDLE ||
        htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1) {
        return;
    }

    uint16_t timestamp =
        (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

    // With both-edge capture, use the pin level to identify the edge.
    bool pinHigh =
        HAL_GPIO_ReadPin(PWM_8_GPIO_Port, PWM_8_Pin) == GPIO_PIN_SET;

    if (pinHigh) {
        if (haveRise && haveFall) {
            // Unsigned 16-bit subtraction handles one counter wrap.
            uint16_t period = (uint16_t)(timestamp - lastRise);

            if (pendingHighTicks > 0 && pendingHighTicks < period) {
                highTicks = pendingHighTicks;
                periodTicks = period;
                lastSampleMs = HAL_GetTick();
            }
        }

        lastRise = timestamp;
        haveRise = true;
        haveFall = false;
    } else if (haveRise) {
        pendingHighTicks = (uint16_t)(timestamp - lastRise);
        haveFall = true;
    }
}

bool pwmInputRead(uint16_t *period, uint16_t *high)
{
    // Copy both values before the interrupt can publish another sample.
    uint32_t savedPrimask = __get_PRIMASK();
    __disable_irq();

    uint16_t capturedPeriod = periodTicks;
    uint16_t capturedHigh = highTicks;
    uint32_t capturedAt = lastSampleMs;

    __set_PRIMASK(savedPrimask);

    if (capturedPeriod == 0 || (HAL_GetTick() - capturedAt) >= 100U) {
        return false;
    }

    *period = capturedPeriod;
    *high = capturedHigh;
    return true;
}