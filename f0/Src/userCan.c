/*
 * userCan.c
 *
 */
#define STRINGIZE_AUX(a) #a
#define STRINGIZE(a) STRINGIZE_AUX(a)
#define CAT_AUX(a, b) a##b
#define CAT(a, b) CAT_AUX(a, b)
#define AUTOGEN_HEADER_NAME(boardName) STRINGIZE(CAT(boardName, _can.h))

#include "stm32f0xx_hal.h"
#include "userCan.h"
#include "stdbool.h"
#include <string.h>
#include AUTOGEN_HEADER_NAME(BOARD_NAME)
#include "can.h"

CanRxMsgTypeDef RxMessage;

void canInit(CAN_HandleTypeDef *hcan)
{
    configCANFilters(hcan);
}

HAL_StatusTypeDef canStartReceiving(CAN_HandleTypeDef *hcan)
{
    hcan->pRxMsg = &RxMessage;

    if (HAL_CAN_Receive_IT(hcan, CAN_FIFO0) != HAL_OK) {
        printf("Error receiving CAN msgs from FIFO0\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
{
    if (parseCANData(hcan->pRxMsg->ExtId, hcan->pRxMsg->Data))
    {
        // TODO: Probably shouldn't call this from an interrupt
        Error_Handler();
    }

    if (HAL_CAN_Receive_IT(hcan, CAN_FIFO0) != HAL_OK) {
        // TODO: Probably shouldn't call this from an interrupt
        Error_Handler();
    }
    /*if (HAL_CAN_Receive_IT(hcan, CAN_FIFO1) != HAL_OK) {*/
        /*// TODO: Probably shouldn't call this from an interrupt*/
        /*Error_Handler();*/
    /*}*/
}


HAL_StatusTypeDef sendCanMessage(int id, int length, uint8_t *data)
{
  const int CAN_TIMEOUT = 100;

  HAL_StatusTypeDef rc = HAL_ERROR;
  CanTxMsgTypeDef        TxMessage;
  hcan.pTxMsg = &TxMessage;

  if (length > 8) {
    return HAL_ERROR;
  }

  printf("Sending CAN message with id %d, length %d, data:\n", id, length);
  for (int i=0; i<length; i++) {
    printf("0x%X ", data[i]);
  }
  printf("\n");

  TxMessage.ExtId = id;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_EXT;
  TxMessage.DLC = length;
  memcpy(TxMessage.Data, data, length);

  rc = HAL_CAN_Transmit(&hcan, CAN_TIMEOUT);
  if (rc != HAL_OK)
  {
    printf("CAN Transmit failed with rc %d\n", rc);
    return HAL_ERROR;
  }

  return rc;
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

uint32_t error = HAL_CAN_ERROR_NONE;
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	// Deal with error

	error = hcan->ErrorCode;
//#define HAL_CAN_ERROR_NONE              ((uint32_t)0x00000000)  /*!< No error             */
//#define HAL_CAN_ERROR_EWG               ((uint32_t)0x00000001)  /*!< EWG error            */
//#define HAL_CAN_ERROR_EPV               ((uint32_t)0x00000002)  /*!< EPV error            */
//#define HAL_CAN_ERROR_BOF               ((uint32_t)0x00000004)  /*!< BOF error            */
//#define HAL_CAN_ERROR_STF               ((uint32_t)0x00000008)  /*!< Stuff error          */
//#define HAL_CAN_ERROR_FOR               ((uint32_t)0x00000010)  /*!< Form error           */
//#define HAL_CAN_ERROR_ACK               ((uint32_t)0x00000020)  /*!< Acknowledgment error */
//#define HAL_CAN_ERROR_BR                ((uint32_t)0x00000040)  /*!< Bit recessive        */
//#define HAL_CAN_ERROR_BD                ((uint32_t)0x00000080)  /*!< LEC dominant         */
//#define HAL_CAN_ERROR_CRC               ((uint32_t)0x00000100)  /*!< LEC transfer error   */

	__HAL_CAN_CANCEL_TRANSMIT(hcan, CAN_TXMAILBOX_0);
	__HAL_CAN_CANCEL_TRANSMIT(hcan, CAN_TXMAILBOX_1);
	__HAL_CAN_CANCEL_TRANSMIT(hcan, CAN_TXMAILBOX_2);
	hcan->Instance->MSR |= CAN_MCR_RESET;

}

