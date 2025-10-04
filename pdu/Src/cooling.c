#include "cooling.h"

#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "FreeRTOS_CLI.h"
#include "debug.h"
#include "pdu_can.h"
#include "watchdog.h"
#include "canReceive.h"
#include "pdu_dtc.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SCALE_TEMP_PERCENT(temp, max_temp) (((temp) - COOLING_AMBIENT_TEMP_C) / ((max_temp) - COOLING_AMBIENT_TEMP_C))

#define CASCADIA_CM200_DERATING_TEMP_C (45.0f) // Inverter can operate for 30s before derating performance
#define EMRAX_228_LC_MAX_TEMP_C (120.0f) 

volatile uint8_t acc_fan_command_override = 0;

void coolingOff(void) {
    DEBUG_PRINT("Turning cooling off\n");
    PUMP_1_DISABLE;
    PUMP_2_DISABLE;
    RADIATOR_DISABLE;
    // ACC_FANS_DISABLE;
}

void coolingOn(void) {
    DEBUG_PRINT("Turning cooling on\n");
    PUMP_1_EN;
    PUMP_2_EN;
    RADIATOR_EN;
    vTaskDelay(pdMS_TO_TICKS(3));   // Lessen in-rush current by introducing a delay
    // ACC_FANS_EN;
}

bool inverterOverheated(void)
{
    return INV_Hot_Spot_Temp > CASCADIA_CM200_DERATING_TEMP_C;
}

bool inverterDeratingPower(void)
{
    return INV_Limit_Coolant_Derating;
}

bool motorOverheated(void)
{
    return INV_Motor_Temp > EMRAX_228_LC_MAX_TEMP_C - 20.0f;
}

void coolingTask(void *pvParameters) {
    if (registerTaskToWatch(COOLING_TASK_ID, 2*COOLING_TASK_PERIOD_MS, false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register cooling task with watchdog!\n");
        Error_Handler();
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();

    bool isCoolingEnabled = false;
    coolingOff();

    while(1)
    {
        // Only cool if motor controller on and EM enabled
        // Should we check if MC is on?
        if (fsmGetState(&mainFsmHandle) == STATE_Motors_On) {
            if (!isCoolingEnabled) {
                isCoolingEnabled = true;
                coolingOn();
            }
            

            if (inverterOverheated())
            {
                static bool sentInvOverheatWarning = false;
                static bool sentInvDeratingWarning = false;
                
                if (!sentInvOverheatWarning)
                {
                    sentInvOverheatWarning = true;
                    sendDTC_WARNING_PDU_Inverter_Overheat();
                    DEBUG_PRINT("Inverter Overheating!\r\n");
                }
                else
                {
                    sentInvOverheatWarning = false;
                }

                if (inverterDeratingPower())
                {
                    if (!sentInvDeratingWarning)
                    {
                        sentInvDeratingWarning = true;
                        DEBUG_PRINT("Inverteris Derating Power\r\n");
                        sendDTC_WARNING_PDU_Inverter_Derating_Power();
                    }
                }
                else 
                {
                    sentInvDeratingWarning = false;
                }
            }
            if (motorOverheated())
            {
                DEBUG_PRINT("Motor Overheated!\r\n");
                sendDTC_FATAL_PDU_Motor_Overheat();
            }
        }
        else 
        {
            if (!acc_fan_command_override) {
                if (isCoolingEnabled) {
                    coolingOff();
                    isCoolingEnabled = false;
                }
            }
        }

        watchdogTaskCheckIn(COOLING_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(COOLING_TASK_PERIOD_MS));
    }
}