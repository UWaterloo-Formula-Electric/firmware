/**
  *****************************************************************************
  * @file    chargerControl.c
  * @author  Richard Matthews
  * @brief   Module containing functions for controlling the battery charger.
  * @details The BMU communicates with the charger over a CAN bus (seperate
  * from the car CAN bus). The BMU sends a message specifying max current and
  * voltage for the charger, and receives feedback messages
  ******************************************************************************
  */

#include "chargerControl.h"
#include "debug.h"
#include "BMU_charger_can.h"
#include "userCan.h"
#include "inttypes.h"
#include <string.h>
#include "watchdog.h"

#define CHARGER_COMM_START_TIMEOUT_MS 3000
#define CHARGER_COMM_START_SEND_PERIOD_MS 100

ChargerStatus mStatus = {0};

HAL_StatusTypeDef chargerInit()
{
#if BOARD_VERSION >= 2 || IS_BOARD_TYPE_NUCLEO_F7
   if (canStart(&CHARGER_CAN_HANDLE) != HAL_OK) {
      ERROR_PRINT("Failed to start charger CAN\n");
   }

   return HAL_OK;
#else
   return HAL_OK;
#endif
}

HAL_StatusTypeDef startChargerCommunication(float maxVoltage, float maxCurrent, uint32_t watchdogTaskId)
{
    ChargerStatus status;
    uint32_t startTickCount = xTaskGetTickCount();
    do {
        sendChargerCommand(maxVoltage, maxCurrent, true /* start charing */);
        watchdogTaskCheckIn(watchdogTaskId);

        if (checkChargerStatus(&status) != HAL_OK) {
            ERROR_PRINT("Failed to get charger status\n");
            return HAL_ERROR;
        }

        if (xTaskGetTickCount() - startTickCount >= pdMS_TO_TICKS(CHARGER_COMM_START_TIMEOUT_MS))
        {
           ERROR_PRINT("Timeout waiting for charger can comms start\n");
           return HAL_ERROR;
        }

        vTaskDelay(pdMS_TO_TICKS(CHARGER_COMM_START_SEND_PERIOD_MS));
    } while (status.OverallState != CHARGER_OK);

    return HAL_OK;
}

HAL_StatusTypeDef sendChargerCommand(float maxVoltage, float maxCurrent, bool startCharging)
{
#if BOARD_VERSION >= 2 || IS_BOARD_TYPE_NUCLEO_F7
   uint32_t maxVoltageInt = maxVoltage / 0.1;
   uint32_t maxCurrentInt = maxCurrent / 0.1;

   MaxChargeVoltageHigh = (maxVoltageInt>>8);
   MaxChargeVoltageLow = (maxVoltageInt & 0xFF);

   MaxChargeCurrentHigh = (maxCurrentInt>>8);
   MaxChargeCurrentLow = (maxCurrentInt & 0xFF);

   /*DEBUG_PRINT("MVH: 0x%X, MVL 0x%X, MCH 0x%X, MCL 0x%X\n", (uint8_t)MaxChargeVoltageHigh,*/
               /*(uint8_t)MaxChargeVoltageLow, (uint8_t)MaxChargeCurrentHigh,*/
               /*(uint8_t)MaxChargeCurrentLow);*/

   StartStopCharge = startCharging?StartStopCharge_ChargeStart:StartStopCharge_ChargeStop;

   /*DEBUG_PRINT("StartStop: %u\n", (uint8_t)StartStopCharge);*/

   /*DEBUG_PRINT("Sending charger command can message\n");*/
   return sendCAN_ChargerCommand();
#else
   return HAL_OK;
#endif
}

void CAN_Msg_ChargeStatus_Callback()
{
   /*DEBUG_PRINT_ISR("Charger status\n");*/

   uint16_t current = (OutputCurrentHigh<<8) | (OutputCurrentLow & 0xFF);
   uint16_t voltage = (OutputVoltageHigh<<8) | (OutputVoltageLow & 0xFF);

   float currentFloat = current * 0.1;
   float voltageFloat = voltage * 0.1;

   mStatus.current = currentFloat;
   mStatus.voltage = voltageFloat;
   mStatus.HWFail = HardwareFailure;
   mStatus.OverTemp = OverTempActive;
   mStatus.InputVoltageStatus = InputVoltageStatus;
   mStatus.StartingStatus = StartingState;
   mStatus.CommunicationState = CommunicationState;

   if (HardwareFailure || OverTempActive || InputVoltageStatus || StartingState
       || CommunicationState)
   {
      mStatus.OverallState = CHARGER_FAIL;
   } else {
      mStatus.OverallState = CHARGER_OK;
   }

   /*DEBUG_PRINT_ISR("Current %f\n", currentFloat);*/
   /*DEBUG_PRINT_ISR("Voltage %f\n", voltageFloat);*/
   /*DEBUG_PRINT_ISR("HW Fail %u, OverTemp %u, InputVoltageStatus %u\n",*/
                   /*(uint16_t)HardwareFailure,*/
                   /*(uint16_t)OverTempActive, (uint16_t)InputVoltageStatus);*/
   /*DEBUG_PRINT_ISR("StartingState %u, CommunicationState %u\n",*/
                   /*(uint16_t)StartingState, (uint16_t)CommunicationState);*/
   /*DEBUG_PRINT_ISR("\n\n");*/
}

HAL_StatusTypeDef checkChargerStatus(ChargerStatus *statusOut)
{
   if (statusOut == NULL) {
      ERROR_PRINT("Got null charger status pointer\n");
      return HAL_ERROR;
   }

   if (mStatus.OverallState != CHARGER_OK)
   {
      ERROR_PRINT("Charger State Fail\n");
      DEBUG_PRINT("Current %f\n", mStatus.current);
      DEBUG_PRINT("Voltage %f\n", mStatus.voltage);
      DEBUG_PRINT("HW Fail %u, OverTemp %u, InputVoltageStatus %u\n",
                      (uint16_t)mStatus.HWFail,
                      (uint16_t)mStatus.OverTemp, (uint16_t)mStatus.InputVoltageStatus);
      DEBUG_PRINT("StartingState %u, CommunicationState %u\n",
                      (uint16_t)mStatus.StartingStatus, (uint16_t)mStatus.CommunicationState);
      DEBUG_PRINT("\n\n");
   }

   memcpy(statusOut, &mStatus, sizeof(ChargerStatus));

   return HAL_OK;
}

