#include "chargerControl.h"
#include "debug.h"
#include "BMU_charger_can.h"
#include "userCan.h"
#include "inttypes.h"


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


HAL_StatusTypeDef sendChargerCommand(float maxVoltage, float maxCurrent, bool startCharging)
{
#if BOARD_VERSION >= 2 || IS_BOARD_TYPE_NUCLEO_F7
   uint32_t maxVoltageInt = maxVoltage / 0.1;
   uint32_t maxCurrentInt = maxCurrent / 0.1;

   MaxChargeVoltageHigh = (maxVoltageInt>>8);
   MaxChargeVoltageLow = (maxVoltageInt & 0xFF);

   MaxChargeCurrentHigh = (maxCurrentInt>>8);
   MaxChargeCurrentLow = (maxCurrentInt & 0xFF);

   DEBUG_PRINT("MVH: 0x%X, MVL 0x%X, MCH 0x%X, MCL 0x%X\n", (uint8_t)MaxChargeVoltageHigh,
               (uint8_t)MaxChargeVoltageLow, (uint8_t)MaxChargeCurrentHigh,
               (uint8_t)MaxChargeCurrentLow);

   StartStopCharge = startCharging?StartStopCharge_ChargeStart:StartStopCharge_ChargeStop;

   DEBUG_PRINT("StartStop: %u\n", (uint8_t)StartStopCharge);

   DEBUG_PRINT("Sending charger command can message\n");
   return sendCAN_ChargerCommand();
#else
   return HAL_OK;
#endif
}

void CAN_Msg_ChargeStatus_Callback()
{
   DEBUG_PRINT_ISR("Charger status\n");

   uint16_t current = (OutputCurrentHigh<<8) | (OutputCurrentLow & 0xFF);
   uint16_t voltage = (OutputVoltageHigh<<8) | (OutputVoltageLow & 0xFF);

   float currentFloat = current * 0.1;
   float voltageFloat = voltage * 0.1;

   DEBUG_PRINT_ISR("Current %f\n", currentFloat);
   DEBUG_PRINT_ISR("Voltage %f\n", voltageFloat);
   DEBUG_PRINT_ISR("HW Fail %u, OverTemp %u, InputVoltageStatus %u\n",
                   (uint16_t)HardwareFailure,
                   (uint16_t)OverTempActive, (uint16_t)InputVoltageStatus);
   DEBUG_PRINT_ISR("StartingState %u, CommunicationState %u\n",
                   (uint16_t)StartingState, (uint16_t)CommunicationState);
   DEBUG_PRINT_ISR("\n\n");
}
