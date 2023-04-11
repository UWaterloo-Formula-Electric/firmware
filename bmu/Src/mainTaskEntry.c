/**
  *****************************************************************************
  * @file    mainTaskEntry.c
  * @author  Richard Matthews
  * @brief   Module containing main task, which is the default task for all
  * boards. It currently blinks the debug LED to indicate the firmware is running
  *****************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "debug.h"
#include "state_of_charge.h"
#include "canReceive.h"
#include "bmu_can.h"
#include "batteries.h"

#define MAIN_TASK_PERIOD 250
#define HEARTBEAT_LED_TOGGLE_PERIOD 1000

void WiCANPublish(void)
{
    static BMU_CAN_CONFIGURED_E bmuCANConfiguredIndex = BMU_CAN_CONFIGURED_MAX_CHARGE_CURRENT;
    WiCANFeedbackBMUEnum = bmuCANConfiguredIndex;
    switch (bmuCANConfiguredIndex)
    {
        case BMU_CAN_CONFIGURED_MAX_CHARGE_CURRENT:
            WiCANFeedbackBMUValue = getMaxChargeCurrent() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackBMU();
            break;
        case BMU_CAN_CONFIGURED_SERIES_CELL_IR:
            WiCANFeedbackBMUValue = getSeriesCellIR() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackBMU();
            break;
        case BMU_CAN_CONFIGURED_STATE_BUS_HV_SEND_PERIOD:
            WiCANFeedbackBMUValue = getStateBusHVSendPeriod();
            sendCAN_WiCAN_FeedbackBMU();
            break;
        case BMU_CAN_CONFIGURED_CAPACITY_STARTUP:
            WiCANFeedbackBMUValue = getCapacityStartup() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackBMU();
            break;
        case BMU_CAN_CONFIGURED_IBUS_INTEGRATED:
            WiCANFeedbackBMUValue = getIBusIntegrated() / WIRELESS_CAN_FLOAT_SCALAR;
            sendCAN_WiCAN_FeedbackBMU();
            break;

        case BMU_CAN_CONFIGURED_COUNT:
        default:
            break;
    }
    bmuCANConfiguredIndex = (bmuCANConfiguredIndex + 1) % BMU_CAN_CONFIGURED_COUNT;
}

void mainTaskFunction(void const * argument)
{
    DEBUG_PRINT("Starting up!!\n");
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xLastLEDToggle = xTaskGetTickCount();

    while (1)
    {
        if (xLastLEDToggle - xTaskGetTickCount() >= HEARTBEAT_LED_TOGGLE_PERIOD)
        {
            HAL_GPIO_TogglePin(DEBUG_LED_PORT, DEBUG_LED_PIN);
            xLastLEDToggle = xTaskGetTickCount();
        }

        WiCANPublish();

        vTaskDelayUntil(&xLastWakeTime, MAIN_TASK_PERIOD);
    }
}
