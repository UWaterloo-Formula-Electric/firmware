/**
  *****************************************************************************
  * @file    canReceive.c
  * @author  Richard Matthews
  * @brief   Module containing callback functions for receiving CAN messages
  * @details Receives DCU HV toggle button press, charge cart messages, and
  * DTCs
  *****************************************************************************
  */

#include "batteries.h"
#include "canReceive.h"

#include "userCan.h"
#include "bsp.h"
#include "debug.h"
#include "boardTypes.h"

#include "controlStateMachine.h"
#include "state_of_charge.h"

#include "bmu_can.h"

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"


void CAN_Msg_DCU_buttonEvents_Callback()
{
	DEBUG_PRINT_ISR("DCU Button events\n");
    if (ButtonHVEnabled) {
		DEBUG_PRINT_ISR("HV Toggle button event\n");
        fsmSendEventISR(&fsmHandle, EV_HV_Toggle);
    }
}

void DTC_Fatal_Callback(BoardIDs board)
{
	DEBUG_PRINT_ISR("DTC Receieved from board %lu \n", board);
    fsmSendEventUrgentISR(&fsmHandle, EV_HV_Fault);
}

uint32_t lastChargeCartHeartbeat = 0;
bool sentChargeStartEvent = false;

void CAN_Msg_ChargeCart_Heartbeat_Callback()
{
    if (!sentChargeStartEvent) {
        if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
            fsmSendEventISR(&fsmHandle, EV_Enter_Charge_Mode);
            sentChargeStartEvent = true;
        }
    }
    lastChargeCartHeartbeat = xTaskGetTickCount();
}

void CAN_Msg_ChargeCart_ButtonEvents_Callback()
{
    if (ButtonChargeStart) {
        fsmSendEventISR(&fsmHandle, EV_Charge_Start);
    }
    if (ButtonChargeStop) {
        fsmSendEventISR(&fsmHandle, EV_Notification_Stop);
    }
    if (ButtonHVEnabled) {
        fsmSendEventISR(&fsmHandle, EV_HV_Toggle);
    }
}

void CAN_Msg_UartOverCanConfig_Callback()
{
	isUartOverCanEnabled = UartOverCanConfigSignal & 0x2;	
}

void CAN_Msg_WiCAN_SetBMU_Callback()
{
    switch (WiCANSetBMUEnum)
    {
        case BMU_CAN_CONFIGURED_MAX_CHARGE_CURRENT:
            setMaxChargeCurrent(WiCANSetBMUValue);
            break;
        
        case BMU_CAN_CONFIGURED_SERIES_CELL_IR:
            setSeriesCellIR(WiCANSetBMUValue);
            break;

        case BMU_CAN_CONFIGURED_STATE_BUS_HV_SEND_PERIOD:
            setStateBusHVSendPeriod(WiCANSetBMUValue);
            break;

        case BMU_CAN_CONFIGURED_CAPACITY_STARTUP:
            setCapacityStartup(WiCANSetBMUValue);
            break;

        case BMU_CAN_CONFIGURED_IBUS_INTEGRATED:
        {
            float newIbusIntegrated = WiCANSetBMUValue * WIRELESS_CAN_FLOAT_SCALAR;
            setIBusIntegrated(newIbusIntegrated);
            break;
        }

        default:
            DEBUG_PRINT("BMU variable %lu not supported\r\n", (uint32_t)WiCANSetBMUEnum);
            break;
        }
}
