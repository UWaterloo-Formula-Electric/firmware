/**
  *****************************************************************************
  * @file    canReceive.c
  * @author  Richard Matthews
  * @brief   Module containing callback functions for receiving CAN messages
  * @details Contains callback functions for receiving CAN messages. Tracks HV
  * and EM enabled states. Receives IMD and AMS Faults for IMD and AMS fault
  * lights
  *****************************************************************************
  */
#include "canReceive.h"

#include "userCan.h"
#include "bsp.h"
#include "debug.h"
#include "controlStateMachine.h"

#include "dcu_can.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "dcu_dtc.h"

/**
 * Get current HV power state, updated from BMU CAN messages
 */
bool getHVState()
{
    return HV_Power_State;
}

/**
 * Get current EM enabled state, updated from VCU CAN messages
 */
bool getEMState()
{
    return EM_State;
}

/**
 * BMU Power state CAN message callback.
 * Signals to the main task the HV power state has changed
 */
void CAN_Msg_BMU_HV_Power_State_Callback()
{
    fsmSendEventISR(&DCUFsmHandle, EV_CAN_Recieve_HV);
}

/**
 * EM power state CAN message callback.
 * Signals to the main task that the EM enabled state has changed
 */
void CAN_Msg_VCU_EM_State_Callback()
{
    fsmSendEventISR(&DCUFsmHandle, EV_CAN_Recieve_EM);
}

/**
 * BMU DTC callback.
 * Used to turn on IMD and AMS fault lights when AMS or IMD fault is received
 */
void CAN_Msg_BMU_DTC_Callback(int DTC_CODE, int DTC_Severity, int DTC_Data)
{
    if (DTC_CODE == FATAL_IMD_Failure) 
    {
        ERROR_PRINT_ISR("Got IMD failure\n");
        IMD_FAIL_LED_ON
    } else if ((DTC_CODE == CRITICAL_CELL_VOLTAGE_LOW)
               || (DTC_CODE == CRITICAL_CELL_VOLTAGE_HIGH)
               || (DTC_CODE == CRITICAL_CELL_TEMP_HIGH)
               || (DTC_CODE == CRITICAL_CELL_TEMP_LOW)) {
        ERROR_PRINT_ISR("Got AMS failure\n");
        AMS_FAIL_LED_ON
    }
}

void DTC_Fatal_Callback(BoardIDs board)
{
	ERROR_PRINT_ISR("DTC fatal received\n");
        fsmSendEventUrgentISR(&DCUFsmHandle, EV_CAN_Recieve_Fatal);
}

void CAN_Msg_UartOverCanConfig_Callback() {
    isUartOverCanEnabled = isUartOverCanEnabled & 0x8;
}
