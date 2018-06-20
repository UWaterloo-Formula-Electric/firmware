/*
 * userCan.c
 *
 */

#include "userCan.h"
#include "stdbool.h"
#include <string.h>
#include AUTOGEN_HEADER_NAME(BOARD_NAME)
#include "can.h"
#include "debug.h"
#include "bsp.h"

#include "userCanF7.h"
#include "userCanF0.h"


#define DTC_SEND_FUNCTION CAT(CAT(sendCAN_,BOARD_NAME),_DTC)

HAL_StatusTypeDef canInit(CAN_HandleTypeDef *hcan)
{
#ifdef BOARD_TYPE_F7
    return F7_canInit(hcan);
#elif defined(BOARD_TYPE_F0)
    return F0_canInit(hcan);
#else
#error canInit not defined for this board type
#endif
}

HAL_StatusTypeDef canStart(CAN_HandleTypeDef *hcan)
{
#ifdef BOARD_TYPE_F7
    return F7_canStart(hcan);
#elif defined(BOARD_TYPE_F0)
    return F0_canStart(hcan);
#else
#error canStart not defined for this board type
#endif
}

HAL_StatusTypeDef sendCanMessage(int id, int length, uint8_t *data)
{
#ifdef BOARD_TYPE_F7
    return F7_sendCanMessage(id, length, data);
#elif defined(BOARD_TYPE_F0)
    return F0_sendCanMessage(id, length, data);
#else
#error Send can message not defined for this board type
#endif
}

HAL_StatusTypeDef sendDTCMessage(int dtcCode, int severity, uint64_t data)
{
    DTC_Data = (float)data;
    DTC_Severity = (float)severity;
    DTC_CODE = (float)dtcCode;
    return DTC_SEND_FUNCTION();
}
/*
 *bool sendCanMessageTimeoutMs(const uint16_t id, const uint8_t *data,
 *                             const uint8_t length, const uint32_t timeout)
 *{
 *    uint32_t beginTime = HAL_GetTick();
 *    bool success = false;
 *    while (HAL_GetTick() < beginTime + timeout && !success)
 *    {
 *        success = sendCanMessage(id, data, length);
 *    }
 *    return success;
 *}
 *
 *bool sendCanMessage(const uint16_t id, const uint8_t *data, const uint8_t length)
 *{
 *    if (length > CAN_MAX_BYTE_LEN)
 *        return false;    // Programmer error
 *    if (initStatus != HAL_OK)
 *        return false;
 *    if (canHandle.Instance->MSR == CAN_MCR_RESET)
 *        return false;
 *
 *    HAL_Delay(1);	// Wait for a previous message to flush out HW
 *
 *    CanTxMsgTypeDef txMessage =
 *    {
 *        .StdId = id,
 *        .IDE = CAN_ID_STD,
 *        .RTR = CAN_RTR_DATA,
 *        .DLC = length
 *    };
 *
 *    uint8_t i;
 *    for (i = 0; i < length; i++)
 *        txMessage.Data[i] = data[i];
 *
 *    canHandle.pTxMsg = &txMessage;
 *    HAL_StatusTypeDef status = HAL_CAN_Transmit_IT(&canHandle);
 *
 *    if (status == HAL_OK)
 *    {
 *        return true;
 *    }
 *    else
 *    {
 *        return false;
 *    }
 *}
 */
