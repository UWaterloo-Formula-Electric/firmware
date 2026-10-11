/**
 *******************************************************************************
 * @file    canReceive.c
 * @brief   HIL's CAN RX callbacks, same layout as the vehicle boards'
 * @details Gen/HIL/Src/HIL_can.c is generated from common/Data/2024CAR.dbc.
 * Its parseCANData() decodes every message that lists HIL as a receiver on
 * one of its signals, writes each signal into a global named after it, and
 * then calls a __weak CAN_Msg_<MessageName>_Callback(). Override that
 * callback here to react to the message. No HIL callbacks exist yet.
 *
 * To receive a message:
 *   1. Add HIL to the receiver list of the signals you need in 2024CAR.dbc
 *   2. Rebuild, then find the generated prototype in Gen/HIL/Inc/HIL_can.h
 *   3. Define it here, without __weak
 *
 * Example for BMU_HV_Power_State, once HIL is added as a receiver of
 * HV_Power_State:
 *
 *   void CAN_Msg_BMU_HV_Power_State_Callback()
 *   {
 *       // HV_Power_State and the HV_Power_State_On/Off enum come from HIL_can.h
 *       DEBUG_PRINT_ISR("BMU HV %s\n",
 *                       HV_Power_State == HV_Power_State_On ? "on" : "off");
 *   }
 *
 * These run in the CAN RX interrupt, so only use the _ISR print and FreeRTOS
 * calls, and keep them short.
 *
 * Callbacks for other message kinds take arguments:
 *   DTC messages:          void CAN_Msg_<Msg>_Callback(int DTC_CODE, int DTC_Severity, int DTC_Data)
 *   Multiplexed messages:  void CAN_Msg_<Msg>_Callback(int baseIndex, int signalsInMessage)
 ******************************************************************************
 */

#include "canReceive.h"

#include "bsp.h"
#include "debug.h"

#include "HIL_can.h"
#include "HIL_dtc.h"

// Overrides the generated __weak configCANFilters(). That one only accepts
// frames addressed to CAN_NODE_ADDRESS, broadcasts and HIL's message groups,
// but a test rig needs to see all traffic on the bus.
void configCANFilters(CAN_HandleTypeDef *canHandle)
{
    CAN_FilterTypeDef filter = {0};
    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(canHandle, &filter) != HAL_OK) {
        Error_Handler();
    }
}

void DTC_Fatal_Callback(BoardIDs board)
{
    // The vehicle boards drive their state machine into a fault state here.
    // HIL has no state machine, and the error LED plus the HIL_DTC frame
    // already flag the error, so there is nothing to do.
    (void)board;
}
