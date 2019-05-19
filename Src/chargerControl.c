#include "chargerControl.h"
#include "debug.h"
#include "BMU_charger_can.h"
#include "userCan.h"


HAL_StatusTypeDef chargerInit()
{
#if BOARD_VERSION >= 2
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
#if BOARD_VERSION >= 2
   uint32_t maxVoltageInt = maxVoltage / 0.1;
   uint32_t maxCurrentInt = maxCurrent / 0.1;

   MaxChargeVoltageHigh = (maxVoltageInt>>8);
   MaxChargeVoltageLow = (maxVoltageInt & 0xFF);

   MaxChargeCurrentHigh = (maxCurrentInt>>8);
   MaxChargeCurrentLow = (maxCurrentInt & 0xFF);

   StartStopCharge = startCharging?StartStopCharge_ChargeStart:StartStopCharge_ChargeStop;

   return sendCAN_ChargerCommand();
#else
   return HAL_OK;
#endif
}

void CAN_Msg_ChargeStatus_Callback()
{
   DEBUG_PRINT_ISR("Charger status\n");

   DEBUG_PRINT_ISR("Current high: %llu, low %llu\n", OutputCurrentHigh, OutputCurrentLow);
   DEBUG_PRINT_ISR("Voltage high: %llu, low %llu\n", OutputVoltageHigh, OutputVoltageLow);
   DEBUG_PRINT_ISR("HW Fail %llu, OverTemp %llu, InputVoltageStatus %llu, StartingState %llu, CommunicationState %llu\n"
                   , HardwareFailure,
                   OverTempActive, InputVoltageStatus,
                   StartingState, CommunicationState);
   DEBUG_PRINT_ISR("\n\n");
}
