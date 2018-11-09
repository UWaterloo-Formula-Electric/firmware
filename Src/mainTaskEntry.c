#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stdbool.h"

#include "bsp.h"
#include "debug.h"
#include "DCU_can.h"

#define EM_BUTTON_DEBOUNCE_MS 100
#define HV_BUTTON_DEBOUNCE_MS 100

#define EM_TOGGLE_BUTTON_EVENT 0x1
#define HV_TOGGLE_BUTTON_EVENT 0x2

// Defined in freertos.c for all board types
extern osThreadId mainTaskHandle;

uint32_t lastEM_Toggle_Ticks = 0;
uint32_t lastHV_Toggle_Ticks = 0;

bool EM_ToggleHigh = false;
bool HV_ToggleHigh = false;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (GPIO_Pin == EM_TOGGLE_BUTTON_PIN) {
        /*DEBUG_PRINT("EM Toggle Pressed\n");*/
        xTaskNotifyFromISR( mainTaskHandle,
                            EM_TOGGLE_BUTTON_EVENT,
                            eSetBits,
                            &xHigherPriorityTaskWoken );
    } else if (GPIO_Pin == HV_TOGGLE_BUTTON_PIN) {
        /*DEBUG_PRINT("HV Toggle Pressed\n");*/
        xTaskNotifyFromISR( mainTaskHandle,
                            HV_TOGGLE_BUTTON_EVENT,
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

        //send CAN Message

        HAL_GPIO_WritePin(DEBUG_LED_PORT, DEBUG_LED_PIN, EM_ToggleHigh);
        return true;
    } else {
        DEBUG_PRINT("Debounced out EM Toggle!\n");
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

        //send CAN Message

        HAL_GPIO_WritePin(DEBUG_LED_PORT, DEBUG_LED_PIN, HV_ToggleHigh);
        return true;
    } else {
        DEBUG_PRINT("Debounced out HV Toggle!\n");
        return false;
    }
}

void mainTaskFunction(void const * argument)
{
    uint32_t buttonStatuses;

    DEBUG_PRINT("Starting up!!\n");

    while (1) {
        xTaskNotifyWait( 0x00,      /* Don't clear any notification bits on entry. */
                         UINT32_MAX, /* Reset the notification value to 0 on exit. */
                         &buttonStatuses, /* Notified value pass out in
                                              buttonStatuses. */
                         portMAX_DELAY );  /* Block indefinitely. */


        DEBUG_PRINT("Got notify event %ld\n", buttonStatuses);

        bool emToggleChange = 0, hvToggleChange = 0;
        if (buttonStatuses & HV_TOGGLE_BUTTON_EVENT) {
            hvToggleChange = HV_TogglePressed();
        }

        if (buttonStatuses & EM_TOGGLE_BUTTON_EVENT) {
            emToggleChange = EM_TogglePressed();
        }

        if (hvToggleChange && emToggleChange) {
            ButtonEMEnabled = 1;
            ButtonHVEnabled = 1;
            DEBUG_PRINT("Sending both changed\n");
            sendCAN_DCU_buttonEvents();
        } else if (hvToggleChange) {
            ButtonEMEnabled = 0;
            ButtonHVEnabled = 1;
            DEBUG_PRINT("Sending hv changed\n");
            sendCAN_DCU_buttonEvents();
        } else if (emToggleChange) {
            ButtonEMEnabled = 1;
            ButtonHVEnabled = 0;
            DEBUG_PRINT("Sending em changed\n");
            sendCAN_DCU_buttonEvents();
        }
    }
}
