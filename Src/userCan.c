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
#include "boardTypes.h"
#include "freertos.h"
#include "queue.h"
#include "task.h"

#include "userCanF7.h"
#include "userCanF0.h"


#define DTC_SEND_FUNCTION CAT(CAT(sendCAN_,BOARD_NAME),_DTC)

#define CAN_SEND_MESSAGE_QUEUE_LENGTH 5

// Private typedef for a struct to store a CAN message to be sent
typedef struct CanMessage_t {
    uint32_t id;
    uint8_t length;
    uint8_t data[8]; // msgs are up to 8 bytes
} CanMessage_t;

QueueHandle_t canMessageSendQueue;
TaskHandle_t canTaskHandle;

HAL_StatusTypeDef canInit(CAN_HandleTypeDef *hcan)
{
#if IS_BOARD_F7_FAMILY
    if (F7_canInit(hcan) != HAL_OK) {
        return HAL_ERROR;
    }
#elif IS_BOARD_F0_FAMILY
    if (F0_canInit(hcan) != HAL_OK) {
        return HAL_ERROR;
    }
#else
#error canInit not defined for this board type
#endif

    canMessageSendQueue = xQueueCreate(CAN_SEND_MESSAGE_QUEUE_LENGTH, sizeof(CanMessage_t));

    if (canMessageSendQueue == NULL) {
        ERROR_PRINT("Failed to create CAN send queue\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef canStart(CAN_HandleTypeDef *hcan)
{
#if IS_BOARD_F7_FAMILY
    return F7_canStart(hcan);
#elif IS_BOARD_F0_FAMILY
    return F0_canStart(hcan);
#else
#error canStart not defined for this board type
#endif
}

HAL_StatusTypeDef sendCanMessage(int id, int length, uint8_t *data)
{
    CanMessage_t msg;
    msg.id = id;
    msg.length = length;
    memcpy(msg.data, data, length);
    if (xQueueSend(canMessageSendQueue, &msg, 0) != pdPASS) {
        ERROR_PRINT("Failed to post can message to send queue\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

void setCanTaskHandle(TaskHandle_t handle)
{
    canTaskHandle = handle;
}

void canTask(void *pvParameters)
{
    CanMessage_t msg;

    while (1) {
        if (xQueueReceive(canMessageSendQueue, &msg, portMAX_DELAY) != pdPASS)
        {
            ERROR_PRINT("Failed to receive from can send queue\n");
            continue;
        }

        HAL_StatusTypeDef rc;
#if IS_BOARD_F7_FAMILY
        rc = F7_sendCanMessage(msg.id, msg.length, msg.data);
#elif IS_BOARD_F0_FAMILY
        rc = F0_sendCanMessage(msg.id, msg.length, msg.data);
#else
#error Send can message not defined for this board type
#endif
        if (rc != HAL_OK)
        {
            ERROR_PRINT("Failed to send can message\n");
            continue;
        }
    }
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
