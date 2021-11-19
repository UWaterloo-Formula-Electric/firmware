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
#define LED_BLINK_PERIOD_MS 500

/**
 * @brief Task function for led task
 * @details LedTask function, which is the entry to the task controlling the
 * dashboard LEDs. This task communicates with the mainTask to blink the LEDs
 * when changing HV/EM state, and turning the LEDs on when HV or EM enabled
 */
void ledTask(void *pvParameters)
{
    while (1)
    {
        switch(fsmGetState(&DCUFsmHandle)){
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
            default:
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_PERIOD_MS));
    }
}
