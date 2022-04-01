/**
  *****************************************************************************
  * @file    ledTask.c
  * @author  Richard Matthews
  * @brief   Module to control LEDs on dashboard
  * @details Contains the ledTask function, which is the entry to the task
  * controlling the dashboard LEDs. This task communicates with the mainTask to
  * blink the LEDs when changing HV/EM state, and turning the LEDs on when HV
  * or EM enabled
  *
  *****************************************************************************
  */
#include "bsp.h"
#include "mainTaskEntry.h"
#include "canReceive.h"

#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "controlStateMachine.h"

#define DEFAULT_LED_BLINK_PERIOD_MS 500
#define ERROR_LED_BLINK_PERIOD_MS 100

#define FLASH_DURATION_MS 150

void selfTestLEDs(void);
void flashLED(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void flashDualLED(GPIO_TypeDef* GPIOx1, uint16_t GPIO_Pin1,
                  GPIO_TypeDef* GPIOx2, uint16_t GPIO_Pin2);

/**
 * @brief Task function for led task
 * @details LedTask function, which is the entry to the task controlling the
 * dashboard LEDs. This task communicates with the mainTask to blink the LEDs
 * when changing HV/EM state, and turning the LEDs on when HV or EM enabled
 */
void ledTask(void *pvParameters)
{
    bool already_errored = false;
    uint32_t blink_period = pdMS_TO_TICKS(DEFAULT_LED_BLINK_PERIOD_MS);

    while (1)
    {
        switch(fsmGetState(&DCUFsmHandle))
        {
            case STATE_Self_Test:
                selfTestLEDs();
                fsmSendEvent(&DCUFsmHandle, EV_Init, 1000);
                break;
            case STATE_HV_Disable:
                HV_LED_OFF;
                EM_LED_OFF;
                break;
            case STATE_HV_Toggle:
                HAL_GPIO_TogglePin(HV_LED_GPIO_Port, HV_LED_Pin);
                EM_LED_OFF;
                break;
            case STATE_HV_Enable:
                HV_LED_ON;
                EM_LED_OFF;
                break;
            case STATE_EM_Toggle:
                HV_LED_ON;
                HAL_GPIO_TogglePin(EM_LED_GPIO_Port, EM_LED_Pin);
                break;
            case STATE_EM_Enable:
                HV_LED_ON;
                EM_LED_ON;
                break;
            case STATE_Failure_Fatal:
                if (!already_errored)
                {
                    HV_LED_ON;
                    EM_LED_OFF;
                    blink_period = pdMS_TO_TICKS(ERROR_LED_BLINK_PERIOD_MS);
                    already_errored = true;
                }

                HAL_GPIO_TogglePin(EM_LED_GPIO_Port, HV_LED_Pin);
                HAL_GPIO_TogglePin(EM_LED_GPIO_Port, EM_LED_Pin);
                break;

            default:
                break;
        }

        vTaskDelay(blink_period);
    }
}

void selfTestLEDs(void)
{
    flashDualLED(MOT_LED_RED_EN_GPIO_Port, MOT_LED_RED_EN_Pin,
                 MOT_LED_GR_EN_GPIO_Port, MOT_LED_GR_EN_Pin);

    flashLED(IMD_LED_EN_GPIO_Port, IMD_LED_EN_Pin);
    flashLED(TC_LED_EN_GPIO_Port, TC_LED_EN_Pin);
    flashLED(HV_LED_EN_GPIO_Port, HV_LED_EN_Pin);
    flashLED(EV_LED_EN_GPIO_Port, EV_LED_EN_Pin);
    flashLED(TV_LED_EN_GPIO_Port, TV_LED_EN_Pin);
    flashDualLED(AMS_LED_RED_EN_GPIO_Port, AMS_LED_RED_EN_Pin,
                 AMS_LED_GR_EN_GPIO_Port, AMS_LED_GR_EN_Pin);
    // OVERTEMP
    flashLED(MC_LED_EN_GPIO_Port, MC_LED_EN_Pin);
}

void flashLED(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(FLASH_DURATION_MS));
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(FLASH_DURATION_MS));
}

void flashDualLED(GPIO_TypeDef* GPIOx1, uint16_t GPIO_Pin1,
                  GPIO_TypeDef* GPIOx2, uint16_t GPIO_Pin2)
{
    HAL_GPIO_WritePin(GPIOx1, GPIO_Pin1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOx2, GPIO_Pin2, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(FLASH_DURATION_MS));
    HAL_GPIO_WritePin(GPIOx1, GPIO_Pin1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOx2, GPIO_Pin2, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(FLASH_DURATION_MS));
}
