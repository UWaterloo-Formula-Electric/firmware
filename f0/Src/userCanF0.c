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

// valid DLC is from 0..8, this value means the msg hasnt been received yet
#define CAN_MESSAGE_DLC_INVALID 9

#define DTC_SEND_FUNCTION CAT(CAT(sendCAN_,BOARD_NAME),_DTC)

CanRxMsgTypeDef RxMessage;
CanRxMsgTypeDef Rx1Message;

HAL_StatusTypeDef F0_canInit(CAN_HandleTypeDef *hcan)
{
    configCANFilters(hcan);
    if (HAL_OK != init_can_driver()) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef F0_canStart(CAN_HandleTypeDef *hcan)
{
    RxMessage.DLC = CAN_MESSAGE_DLC_INVALID;
    Rx1Message.DLC = CAN_MESSAGE_DLC_INVALID;
    hcan->pRxMsg = &RxMessage;
    hcan->pRx1Msg = &Rx1Message;

    if (HAL_CAN_Receive_IT(hcan, CAN_FIFO0) != HAL_OK) {
        ERROR_PRINT("Error receiving CAN msgs from FIFO0\n");
        return HAL_ERROR;
    }
    if (HAL_CAN_Receive_IT(hcan, CAN_FIFO1) != HAL_OK) {
        ERROR_PRINT("Error receiving CAN msgs from FIFO0\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
{
  HAL_StatusTypeDef rc = 0;
    if (hcan->pRxMsg->DLC != CAN_MESSAGE_DLC_INVALID) {
        hcan->pRxMsg->DLC = CAN_MESSAGE_DLC_INVALID;

        HAL_CAN_StateTypeDef canState = HAL_CAN_GetState(hcan);
        if (canState == HAL_CAN_STATE_BUSY_RX0 ||
            canState == HAL_CAN_STATE_BUSY_TX_RX0 ||
            canState == HAL_CAN_STATE_BUSY_RX0_RX1 ||
            canState == HAL_CAN_STATE_BUSY_TX_RX0_RX1)
        {
            ERROR_PRINT_ISR("DLC indicates rx on fifo0, but RX0 is busy. This shouldn't happen\n");
            Error_Handler();
        }

        if (parseCANData(hcan->pRxMsg->ExtId, hcan->pRxMsg->Data))
        {
            /*ERROR_PRINT_ISR("Failed to parse CAN message id %lu", hcan->pRxMsg->ExtId);*/
        }

        rc = HAL_CAN_Receive_IT(hcan, CAN_FIFO0);
        if (rc != HAL_OK) {
            ERROR_PRINT_ISR("Failed to start new CAN receive IT, rc: %d\n", rc);
            ERROR_PRINT_ISR("Parsed msg id 0x%lX, received on FIFO0\n", hcan->pRxMsg->ExtId);
            if (rc == HAL_BUSY) {
                switch(hcan->State)
                {
                    case(HAL_CAN_STATE_BUSY_TX_RX0):
                        hcan->State = HAL_CAN_STATE_BUSY_TX;
                        break;
                    case(HAL_CAN_STATE_BUSY_RX0_RX1):
                        hcan->State = HAL_CAN_STATE_BUSY_RX1;
                        break;
                    case(HAL_CAN_STATE_BUSY_TX_RX0_RX1):
                        hcan->State = HAL_CAN_STATE_BUSY_TX_RX1;
                        break;
                    default: /* HAL_CAN_STATE_BUSY_RX0 */
                        hcan->State = HAL_CAN_STATE_READY;
                        break;
                }
                rc = HAL_CAN_Receive_IT(hcan, CAN_FIFO0);
                if (rc != HAL_OK) {
                    ERROR_PRINT_ISR("Failed to start new CAN receive IT, rc: %d\n", rc);
                }
            }

            /*Error_Handler();*/
        }
    } else {
        hcan->pRx1Msg->DLC = CAN_MESSAGE_DLC_INVALID;

        HAL_CAN_StateTypeDef canState = HAL_CAN_GetState(hcan);
        if (canState == HAL_CAN_STATE_BUSY_RX1 ||
            canState == HAL_CAN_STATE_BUSY_TX_RX1 ||
            canState == HAL_CAN_STATE_BUSY_RX0_RX1 ||
            canState == HAL_CAN_STATE_BUSY_TX_RX0_RX1)
        {
            ERROR_PRINT_ISR("DLC indicates rx on fifo1, but RX1 is busy. This shouldn't happen\n");
            Error_Handler();
        }

        if (parseCANData(hcan->pRxMsg->ExtId, hcan->pRxMsg->Data))
        {
            /*ERROR_PRINT_ISR("Failed to parse CAN message id %lu", hcan->pRxMsg->ExtId);*/
        }

        rc = HAL_CAN_Receive_IT(hcan, CAN_FIFO1);
        if (rc != HAL_OK) {
            ERROR_PRINT_ISR("Failed to start new CAN receive IT, rc: %d, state = %d\n", rc,
                            hcan->State);
            ERROR_PRINT_ISR("Parsed msg id 0x%lX, received on FIFO1\n", hcan->pRxMsg->ExtId);
            /*Error_Handler();*/
        }
    }
}


HAL_StatusTypeDef F0_sendCanMessage(int id, int length, uint8_t *data)
{
  const int CAN_TIMEOUT = 100;

  HAL_StatusTypeDef rc = HAL_ERROR;
  CanTxMsgTypeDef        TxMessage;
  CAN_HANDLE.pTxMsg = &TxMessage;

  if (length > 8) {
    return HAL_ERROR;
  }

  /*printf("Sending CAN message with id %d, length %d, data:\n", id, length);*/
  /*for (int i=0; i<length; i++) {*/
    /*printf("0x%X ", data[i]);*/
  /*}*/
  /*printf("\n");*/

  TxMessage.ExtId = id;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_EXT;
  TxMessage.DLC = length;
  memcpy(TxMessage.Data, data, length);

  rc = HAL_CAN_Transmit(&CAN_HANDLE, CAN_TIMEOUT);
  if (rc != HAL_OK)
  {
    ERROR_PRINT("CAN Transmit failed with rc %d\n", rc);
    return HAL_ERROR;
  }

  return rc;
}

// Implement this here, as F0 driver doesn't implement this (for our version at
// least)
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *hcan)
{
  uint32_t freelevel = 0U;

  /* Check Tx Mailbox 0 status */
  if ((hcan->Instance->TSR & CAN_TSR_TME0) != 0U)
  {
    freelevel++;
  }

  /* Check Tx Mailbox 1 status */
  if ((hcan->Instance->TSR & CAN_TSR_TME1) != 0U)
  {
    freelevel++;
  }

  /* Check Tx Mailbox 2 status */
  if ((hcan->Instance->TSR & CAN_TSR_TME2) != 0U)
  {
    freelevel++;
  }

  /* Return Tx Mailboxes free level */
  return freelevel;
}

uint32_t error = HAL_CAN_ERROR_NONE;
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	// Deal with error

	error = hcan->ErrorCode;
        ERROR_PRINT_ISR("Error in CAN driver %lu!!\n", error);
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

