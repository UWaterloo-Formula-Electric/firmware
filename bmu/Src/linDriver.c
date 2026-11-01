/* Transceiver Datasheet: https://www.ti.com/lit/ds/symlink/tlin1029-q1.pdf */

#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "linDriver.h"

// Buffer aligned with D-cache for cache management
static uint8_t rxBuffer[LIN_RX_BUFFER_SIZE] __attribute__((aligned(32)));

static uint8_t calcChecksum(
    uint8_t id,
    uint8_t pid,
    const uint8_t *data,
    uint8_t dataLength) {
    // Classic checksum for diagnostic frames, enhanced otherwise
    uint16_t sum = (id == 0x3CU || id == 0x3DU) ? 0U : pid;

    for (uint8_t i = 0; i < dataLength; ++i) {
        sum += data[i];
        if (sum > 0xFFU) {
            sum -= 0xFFU;
        }
    }
    return (uint8_t)~sum;
}

static void setProtectedID(LIN_Frame_t *frame) {
    uint8_t id = frame->id;
    uint8_t p0 = (id ^ (id >> 1U) ^ (id >> 2U) ^ (id >> 4U)) & 1U;
    uint8_t p1 = (~((id >> 1U) ^ (id >> 3U) ^ (id >> 4U) ^ (id >> 5U))) & 1U;

    frame->pid = id | (p0 << 6U) | (p1 << 7U);
}

HAL_StatusTypeDef buildFrame(
    LIN_Frame_t *frame,
    const uint8_t *data,
    uint8_t dataLength) {
    if (frame == NULL) {
        ERROR_PRINT("Failed to build frame from null pointer");
        return HAL_ERROR;
    }
    if (frame->id > LIN_MAX_FRAME_ID) {
        ERROR_PRINT("Failed to build frame with invalid ID");
        return HAL_ERROR;
    }
    if (!frame->isRxBuffer && data == NULL) {
        ERROR_PRINT("Failed to build frame with null data");
        return HAL_ERROR;
    }
    if (dataLength == 0U || dataLength > LIN_MAX_DATA_SIZE) {
        ERROR_PRINT("Failed to build frame with invalid data length");
        return HAL_ERROR;
    }

    setProtectedID(frame);

    frame->buffer[0] = LIN_SYNC_BYTE;
    frame->buffer[1] = frame->pid;
    frame->length = LIN_HEADER_SIZE + dataLength + 1U;

    if (!frame->isRxBuffer) {
        memcpy(frame->buffer + LIN_HEADER_SIZE, data, dataLength);
        frame->buffer[frame->length - 1U] =
            calcChecksum(frame->id, frame->pid, data, dataLength);
    }

    // Max frame time = 1.4 * (34 + 10 * (N + 1)) bit times
    uint32_t baud = LIN_UART_HANDLE->Init.BaudRate;
    frame->duration_us = (14UL * (34UL + 10UL * (dataLength + 1UL)) * 100000UL) / baud;
    frame->rxValid = false;

    return HAL_OK;
}

static HAL_StatusTypeDef txBreak(void) {
    UART_HandleTypeDef *huart = LIN_UART_HANDLE;

    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_LBDF | UART_CLEAR_FEF);
    uint32_t start = __HAL_TIM_GET_COUNTER(LIN_TIM_HANDLE);

    if (HAL_LIN_SendBreak(huart) != HAL_OK) {
        ERROR_PRINT("Failed to send LIN break");
        return HAL_ERROR;
    }

    // Wait for the transceiver to see the echoed break
    while (!__HAL_UART_GET_FLAG(huart, UART_FLAG_LBDF) &&
           !__HAL_UART_GET_FLAG(huart, UART_FLAG_FE)) {
        if (__HAL_TIM_GET_COUNTER(LIN_TIM_HANDLE) - start > LIN_BREAK_TIMEOUT_US) {
            ERROR_PRINT("Failed to send break before timeout");
            return HAL_TIMEOUT;
        }
    }

    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_LBDF | UART_CLEAR_FEF);
    // Drop the break delimiter
    __HAL_UART_SEND_REQ(huart, UART_RXDATA_FLUSH_REQUEST);
    return HAL_OK;
}

static HAL_StatusTypeDef waitForDma(uint32_t duration_us) {
    UART_HandleTypeDef *huart = LIN_UART_HANDLE;
    TickType_t start = xTaskGetTickCount();
    TickType_t limit = pdMS_TO_TICKS((duration_us + 999U) / 1000U) + 2U;

    while (huart->RxState != HAL_UART_STATE_READY || huart->gState != HAL_UART_STATE_READY) {
        if ((TickType_t)(xTaskGetTickCount() - start) > limit) {
            HAL_UART_Abort(huart);
            ERROR_PRINT("Slave failed to receive break before timeout");
            return HAL_TIMEOUT;
        }
        vTaskDelay(1);
    }
    return HAL_OK;
}

HAL_StatusTypeDef transferFrame(LIN_Frame_t *frame) {
    UART_HandleTypeDef *huart = LIN_UART_HANDLE;

    if (frame == NULL || frame->length <= LIN_HEADER_SIZE ||
        frame->length > sizeof(frame->buffer)) {
        ERROR_PRINT("Failed to transfer invalid frame");
        return HAL_ERROR;
    }

    frame->rxValid = false;
    huart->ErrorCode = HAL_UART_ERROR_NONE;

    HAL_StatusTypeDef status = txBreak();
    if (status != HAL_OK) {
        return status;
    }

    uint16_t txLength = frame->isRxBuffer ? LIN_HEADER_SIZE : frame->length;

    if (HAL_UART_Receive_DMA(huart, rxBuffer, frame->length) != HAL_OK) {
        ERROR_PRINT("Failed to start DMA reception");
        return HAL_ERROR;
    }

    // Write the frame to RAM for transmission
    // Avoid address alignment errors
    SCB_CleanDCache_by_Addr((uint32_t *)((uint32_t)frame->buffer & ~31U), 64U);

    if (HAL_UART_Transmit_DMA(huart, frame->buffer, txLength) != HAL_OK) {
        ERROR_PRINT("Failed to start DMA transmission");
        HAL_UART_Abort(huart);
        return HAL_ERROR;
    }

    status = waitForDma(frame->duration_us);

    if (status != HAL_OK) {
        ERROR_PRINT("Slave failed to receive break before timeout");
        return status;
    }

    if (huart->ErrorCode != HAL_UART_ERROR_NONE) {
        ERROR_PRINT("Failed to transmit frame due to UART error");
        return HAL_ERROR;
    }

    // Clear old data from cache
    SCB_InvalidateDCache_by_Addr((uint32_t *)rxBuffer, sizeof(rxBuffer));

    if (memcmp(rxBuffer, frame->buffer, txLength) != 0) {
        ERROR_PRINT("Mismatch between transmitted frame and echo");
        return HAL_ERROR;
    }

    if (frame->isRxBuffer) {
        uint8_t dataLength = frame->length - LIN_HEADER_SIZE - 1U;
        uint8_t expectedChecksum = calcChecksum(
            frame->id,
            frame->pid,
            rxBuffer + LIN_HEADER_SIZE,
            dataLength);

        if (expectedChecksum != rxBuffer[frame->length - 1U]) {
            ERROR_PRINT("Received frame with invalid checksum");
            return HAL_ERROR;
        }
        memcpy(frame->buffer + LIN_HEADER_SIZE, rxBuffer + LIN_HEADER_SIZE,
               frame->length - LIN_HEADER_SIZE);
        frame->rxValid = true;
    }
    return HAL_OK;
}