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
#include "freertos.h"
#include "task.h"

#define DTC_SEND_FUNCTION CAT(CAT(sendCAN_,BOARD_NAME),_DTC)

HAL_StatusTypeDef F7_canInit(CAN_HandleTypeDef *hcan)
{
    configCANFilters(hcan);
    if (HAL_OK != init_can_driver()) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef F7_canStart(CAN_HandleTypeDef *hcan)
{
    if (HAL_CAN_Start(hcan) != HAL_OK) {
        ERROR_PRINT("Failed to start CAN!\n");
        return HAL_ERROR;
    }

    if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        ERROR_PRINT("Error starting to listen for CAN msgs from FIFO0\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef   RxHeader;
    uint8_t               RxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK)
    {
        ERROR_PRINT_ISR("Failed to receive CAN message from FIFO0\n");
        Error_Handler();
    }

    if (parseCANData(RxHeader.ExtId, RxData) != HAL_OK) {
        ERROR_PRINT_ISR("Failed to parse CAN message id %lu", RxHeader.ExtId);
    }
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef   RxHeader;
    uint8_t               RxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData) != HAL_OK)
    {
        ERROR_PRINT_ISR("Failed to receive CAN message from FIFO1\n");
        Error_Handler();
    }

    if (parseCANData(RxHeader.ExtId, RxData) != HAL_OK) {
        ERROR_PRINT_ISR("Failed to parse CAN message id %lu", RxHeader.ExtId);
    }
}

/*
 *void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
 *{
 *    if (hcan->pRxMsg->DLC != CAN_MESSAGE_DLC_INVALID) {
 *        hcan->pRxMsg->DLC = CAN_MESSAGE_DLC_INVALID;
 *
 *        HAL_CAN_StateTypeDef canState = HAL_CAN_GetState(hcan);
 *        if (canState == HAL_CAN_STATE_BUSY_RX0 ||
 *            canState == HAL_CAN_STATE_BUSY_TX_RX0 ||
 *            canState == HAL_CAN_STATE_BUSY_RX0_RX1 ||
 *            canState == HAL_CAN_STATE_BUSY_TX_RX0_RX1)
 *        {
 *            ERROR_PRINT("DLC indicates rx on fifo0, but RX0 is busy. This shouldn't happen\n");
 *            Error_Handler();
 *        }
 *
 *        if (parseCANData(hcan->pRxMsg->ExtId, hcan->pRxMsg->Data))
 *        {
 *            // TODO: Probably shouldn't call this from an interrupt
 *            Error_Handler();
 *        }
 *
 *        if (HAL_CAN_Receive_IT(hcan, CAN_FIFO0) != HAL_OK) {
 *            // TODO: Probably shouldn't call this from an interrupt
 *            Error_Handler();
 *        }
 *    } else {
 *        hcan->pRx1Msg->DLC = CAN_MESSAGE_DLC_INVALID;
 *
 *        HAL_CAN_StateTypeDef canState = HAL_CAN_GetState(hcan);
 *        if (canState == HAL_CAN_STATE_BUSY_RX1 ||
 *            canState == HAL_CAN_STATE_BUSY_TX_RX1 ||
 *            canState == HAL_CAN_STATE_BUSY_RX0_RX1 ||
 *            canState == HAL_CAN_STATE_BUSY_TX_RX0_RX1)
 *        {
 *            ERROR_PRINT("DLC indicates rx on fifo1, but RX1 is busy. This shouldn't happen\n");
 *            Error_Handler();
 *        }
 *
 *        if (HAL_CAN_Receive_IT(hcan, CAN_FIFO1) != HAL_OK) {
 *            // TODO: Probably shouldn't call this from an interrupt
 *            Error_Handler();
 *        }
 *    }
 *}
 */


#define F7_CAN_SEND_TIMEOUT_MS 2
HAL_StatusTypeDef F7_sendCanMessage(int id, int length, uint8_t *data)
{
    HAL_StatusTypeDef     rc = HAL_ERROR;
    CAN_TxHeaderTypeDef   TxHeader = {0};
    uint32_t              TxMailbox;

    if (length > 8) {
        return HAL_ERROR;
    }

    /*printf("Sending CAN message with id %d, length %d, data:\n", id, length);*/
    /*for (int i=0; i<length; i++) {*/
    /*printf("0x%X ", data[i]);*/
    /*}*/
    /*printf("\n");*/

    TxHeader.ExtId = id;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_EXT;
    TxHeader.DLC = length;
    TxHeader.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_GetTxMailboxesFreeLevel(&CAN_HANDLE) == 0) {
        ERROR_PRINT("Can transmit failed, no free mailboxes\n");
        return HAL_ERROR;
    }

    rc = HAL_CAN_AddTxMessage(&CAN_HANDLE, &TxHeader, data, &TxMailbox);
    if (rc != HAL_OK)
    {
        ERROR_PRINT("CAN Transmit failed with rc %d\n", rc);
        if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
            vTaskDelay(pdMS_TO_TICKS(F7_CAN_SEND_TIMEOUT_MS));
        } else {
            HAL_Delay(F7_CAN_SEND_TIMEOUT_MS);
        }
        return HAL_ERROR;
    }

    return rc;
}

uint32_t error = HAL_CAN_ERROR_NONE;
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	// Deal with error

	error = hcan->ErrorCode;
        ERROR_PRINT_ISR("Error in CAN driver!!\n");
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

	/*__HAL_CAN_CANCEL_TRANSMIT(hcan, CAN_TXMAILBOX_0);*/
	/*__HAL_CAN_CANCEL_TRANSMIT(hcan, CAN_TXMAILBOX_1);*/
	/*__HAL_CAN_CANCEL_TRANSMIT(hcan, CAN_TXMAILBOX_2);*/
	/*hcan->Instance->MSR |= CAN_MCR_RESET;*/

}

