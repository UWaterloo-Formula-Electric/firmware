#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stdbool.h"

#include "bsp.h"
#include "debug.h"
#include "DCU_can.h"
#include "userCan.h"
#include "watchdog.h"
#include "mainTaskEntry.h"
#include "canReceive.h"

#define EM_BUTTON_DEBOUNCE_MS 200
#define HV_BUTTON_DEBOUNCE_MS 200

#define MAIN_TASK_ID 1

// The task just waits for buttons, so doesn't really need a period
// Put one so it can work with the watchdog though
// This is just the amount of time it will wait for events
// It then refreshes the watchdog, and either handles events or goes back to
// waiting
#define MAIN_TASK_PERIOD 100

// Defined in freertos.c for all board types
extern osThreadId mainTaskHandle;

uint32_t lastEM_Toggle_Ticks = 0;
uint32_t lastHV_Toggle_Ticks = 0;

bool EM_ToggleHigh = false;
bool HV_ToggleHigh = false;

bool waitingForHVChange = false;
bool waitingForEMChange = false;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    /*DEBUG_PRINT_ISR("Callback\n");*/

    if (GPIO_Pin == EM_TOGGLE_BUTTON_PIN) {
        DEBUG_PRINT_ISR("EM Toggle Pressed\n");
        xTaskNotifyFromISR( mainTaskHandle,
                            (1<<EM_TOGGLE_BUTTON_EVENT),
                            eSetBits,
                            &xHigherPriorityTaskWoken );
    } else if (GPIO_Pin == HV_TOGGLE_BUTTON_PIN) {
        DEBUG_PRINT_ISR("HV Toggle Pressed\n");
        xTaskNotifyFromISR( mainTaskHandle,
                            (1<<HV_TOGGLE_BUTTON_EVENT),
                            eSetBits,
                            &xHigherPriorityTaskWoken );
    } else {
        ERROR_PRINT("Some unkown button pressed\n");
    }

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

// Handle press of EM Toggle Button
// return true if change in EM Toggle state
bool EM_TogglePressed() {
    if (xTaskGetTickCount() - lastEM_Toggle_Ticks
        >= pdMS_TO_TICKS(EM_BUTTON_DEBOUNCE_MS)) {
        DEBUG_PRINT("Pressed EM Toggle\n");
        lastEM_Toggle_Ticks = xTaskGetTickCount();

        EM_ToggleHigh = !EM_ToggleHigh;

        return true;
    } else {
        /*DEBUG_PRINT("Debounced out EM Toggle!\n");*/
        return false;
    }
}

// Handle press of EM Toggle Button
// return true if change in HV Toggle state
bool HV_TogglePressed() {
    if (xTaskGetTickCount() - lastHV_Toggle_Ticks
        >= pdMS_TO_TICKS(HV_BUTTON_DEBOUNCE_MS)) {
        DEBUG_PRINT("Pressed HV Toggle\n");
        lastHV_Toggle_Ticks = xTaskGetTickCount();

        HV_ToggleHigh = !HV_ToggleHigh;

        return true;
    } else {
        /*DEBUG_PRINT("Debounced out HV Toggle!\n");*/
        return false;
    }
}

void mainTaskFunction(void const * argument)
{
    uint32_t notification;

    DEBUG_PRINT("Starting up!!\n");
    if (canStart(&CAN_HANDLE) != HAL_OK) {
        ERROR_PRINT("Failed to start can\n");
        Error_Handler();
    }


    if (registerTaskToWatch(MAIN_TASK_ID, 2*pdMS_TO_TICKS(MAIN_TASK_PERIOD), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register main task with watchdog!\n");
        Error_Handler();
    }

    while (1) {
        xTaskNotifyWait( 0x00,      /* Don't clear any notification bits on entry. */
                         UINT32_MAX, /* Reset the notification value to 0 on exit. */
                         &notification, /* Notified value pass out in
                                              notification. */
                         MAIN_TASK_PERIOD ); /* See comment on main task period */


        if (notification & (1<<EM_TOGGLE_BUTTON_EVENT)
            || notification & (1<<HV_TOGGLE_BUTTON_EVENT))
        {
            DEBUG_PRINT("Got notify event %ld\n", notification);

            bool emToggleChange = 0, hvToggleChange = 0;
            if (notification & (1<<HV_TOGGLE_BUTTON_EVENT)) {
                hvToggleChange = HV_TogglePressed();
            }

            if (notification & (1<<EM_TOGGLE_BUTTON_EVENT)) {
                emToggleChange = EM_TogglePressed();
            }

            if (hvToggleChange && emToggleChange) {
                ButtonEMEnabled = 1;
                ButtonHVEnabled = 1;

                waitingForHVChange = true;
                waitingForEMChange = true;

                DEBUG_PRINT("Sending both changed\n");
                sendCAN_DCU_buttonEvents();
            } else if (hvToggleChange) {
                ButtonEMEnabled = 0;
                ButtonHVEnabled = 1;

                waitingForHVChange = true;

                DEBUG_PRINT("Sending hv changed\n");
                sendCAN_DCU_buttonEvents();
            } else if (emToggleChange) {
                ButtonEMEnabled = 1;
                ButtonHVEnabled = 0;

                waitingForEMChange = true;

                DEBUG_PRINT("Sending em changed\n");
                sendCAN_DCU_buttonEvents();
            }
        } else if ((notification & (1<<HV_ENABLED_NOTIFICATION))
                   || (notification & (1<<HV_DISABLED_NOTIFICATION)))
        {
            waitingForHVChange = false;
        } else if ((notification & (1<<EM_ENABLED_NOTIFICATION))
                   || (notification & (1<<EM_DISABLED_NOTIFICATION)))
        {
            waitingForEMChange = false;
        }

        watchdogTaskCheckIn(MAIN_TASK_ID);
    }
}
