/* Transceiver Datasheet: https://www.ti.com/lit/ds/symlink/tlin1029-q1.pdf */

#include "debug.h"
#include "linDriver.h"

LIN_Driver_t linDriver = {
    .startTick = 0,
    .endTick = 0
};

static uint32_t elapsed_us(void) {
    return __HAL_TIM_GET_COUNTER(LIN_TIM_HANDLE) - linDriver.startTick;
}

static uint8_t getDMABufferByte(uint8_t index) {
    return linDriver.dmaBuffer[index % LIN_DMA_BUFFER_SIZE];
}

HAL_StatusTypeDef buildFrame(
    LIN_Frame_t *frame,
    uint8_t *data,
    uint8_t length) {
    if (frame == NULL) {
        ERROR_PRINT("Failed to build frame from null pointer");
        return HAL_ERROR;
    }
    else if (frame->id > LIN_MAX_FRAME_ID) {
        ERROR_PRINT("Failed to build frame with invalid ID");
        return HAL_ERROR;
    }
    else if (!frame->isRxBuffer && data == NULL) {
        ERROR_PRINT("Failed to build frame with null data");
    }
    else if (length == 0 || length > LIN_MAX_DATA_SIZE) {
        ERROR_PRINT("Failed to build frame with invalid data length");
        return HAL_ERROR;
    }

    HAL_StatusTypeDef pidCalcStatus = setProtectedID(frame);

    if (pidCalcStatus == HAL_ERROR) {
        ERROR_PRINT("Failed to set frame PID");
        return HAL_ERROR;
    }

    frame->buffer[0] = LIN_SYNC_BYTE;
    frame->buffer[1] = frame->pid;
    frame->length = LIN_HEADER_SIZE + length + 1;

    if (!frame->isRxBuffer) {
        memcpy(frame->buffer + LIN_HEADER_SIZE, data, length);
        
        HAL_StatusTypeDef cksCalcStatus = setChecksum(frame);

        if (cksCalcStatus == HAL_ERROR) {
            ERROR_PRINT("Failed to set frame checksum");
            return HAL_ERROR;
        }
    }

    frame->rxValid = false;

    return HAL_OK;
}

HAL_StatusTypeDef setProtectedID(LIN_Frame_t *frame) {
    if (frame == NULL) {
        ERROR_PRINT("Faild to calculate PID from null pointer");
        return HAL_ERROR;
    }
    else if (frame->id > LIN_MAX_FRAME_ID) {
        ERROR_PRINT("Failed to calculate PID from invalid ID");
        return HAL_ERROR;
    }

    uint8_t id = frame->id;
    uint8_t po = (id ^ (id >> 1U) ^ (id >> 2U) ^ (id >> 4U)) & 1U;
    uint8_t p1 = (~((id >> 1U) ^ (id >> 3U) ^ (id >> 4U) ^ (id >> 5U))) & 1U;

    frame->pid = id | (po << 6U) | (p1 << 7U);

    return HAL_OK;
}

HAL_StatusTypeDef setChecksum(LIN_Frame_t *frame) {
    if (frame == NULL) {
        ERROR_PRINT("Failed to calculate checksum from null pointer");
        return HAL_ERROR;
    }
    else if (frame->length == 0 ||
        frame->length > LIN_MAX_RESPONSE_SIZE + LIN_HEADER_SIZE) {
        ERROR_PRINT("Failed to calculate checksum for frame with invalid length");
        return HAL_ERROR;
    }

    uint16_t sum = 0;

    if (!(frame->id == 0x3C || frame->id == 0x3D)) {
        sum = frame->pid;
    }

    for (uint8_t i = LIN_HEADER_SIZE; i < frame->length - 1U; ++i) {
        sum += frame->buffer[i];

        if (sum > 0xFF) {
            sum -= 0xFF;
        }
    }

    frame->buffer[frame->length - LIN_HEADER_SIZE] = ~(sum & 0xFF);

    return HAL_OK;
}

HAL_StatusTypeDef setUpNextFrame(LIN_Frame_t *frame) {
    if (frame == NULL) {
        ERROR_PRINT("Failed to set up driver for null frame");
        return HAL_ERROR;
    }
    linDriver.startTick = 0;
    linDriver.endTick = 0;

    memcpy(&linDriver.activeFrame, frame, sizeof(LIN_Frame_t));
    linDriver.activeFrame.rxValid = false;

    return HAL_OK;
}

HAL_StatusTypeDef txBreak(void) {
    linDriver.startTick = __HAL_TIM_GET_COUNTER(LIN_TIM_HANDLE);
    HAL_StatusTypeDef breakStatus = HAL_LIN_SendBreak(LIN_UART_HANDLE);

    if (breakStatus != HAL_OK) {
        ERROR_PRINT("Failed to send LIN break byte");
        return breakStatus;
    }
    while (elapsed_us() < linDriver.activeFrame.duration_us &&
            !__HAL_UART_GET_FLAG(LIN_UART_HANDLE, UART_FLAG_LBDF) &&
            !__HAL_UART_GET_FLAG(LIN_UART_HANDLE, UART_FLAG_FE)) {
        linDriver.endTick = __HAL_TIM_GET_COUNTER(LIN_TIM_HANDLE);
    }

    if (elapsed_us() >= linDriver.activeFrame.duration_us) {
        ERROR_PRINT("Failed to send break before timeout");
        return HAL_ERROR;
    }

    __HAL_UART_CLEAR_FLAG(LIN_UART_HANDLE, UART_CLEAR_LBDF | UART_CLEAR_FEF);
    return HAL_OK;
}

HAL_StatusTypeDef trxFrame(void) {
    if (elapsed_us() >= linDriver.activeFrame.duration_us) {
        ERROR_PRINT("Failed to transmit frame before timeout");
        return HAL_ERROR;
    }

    if (linDriver.activeFrame.isRxBuffer) {
        HAL_StatusTypeDef rxStatus = HAL_UART_Receive_DMA(
            LIN_UART_HANDLE,
            linDriver.dmaBuffer,
            LIN_DMA_BUFFER_SIZE);
        if (rxStatus != HAL_OK) {
            ERROR_PRINT("Failed to initiate armed DMA reception path");
            return rxStatus;
        }
    }

    HAL_StatusTypeDef headerStatus = HAL_UART_Transmit_DMA(
        LIN_UART_HANDLE,
        linDriver.activeFrame.buffer,
        LIN_HEADER_SIZE
    );

    if (headerStatus != HAL_OK) {
        ERROR_PRINT("Failed to transmit frame header");
        if (linDriver.activeFrame.isRxBuffer) {
            HAL_UART_DMAStop(LIN_UART_HANDLE);
        }
        return headerStatus;
    }

    if (linDriver.activeFrame.isRxBuffer) {
        while (elapsed_us() < linDriver.activeFrame.duration_us) {
            uint16_t bufferLeft = __HAL_DMA_GET_COUNTER(*LIN_UART_HANDLE.hdmarx);
            uint16_t bufferReceived = LIN_DMA_BUFFER_SIZE - bufferLeft;

            if (bufferReceived >= linDriver.activeFrame.length) {
                HAL_UART_DMAStop(LIN_UART_HANDLE);
                break;
            }

            linDriver.endTick = __HAL_TIM_GET_COUNTER(LIN_TIM_HANDLE);
        }

        if (elapsed_us() >= linDriver.activeFrame.duration_us) {
            ERROR_PRINT("Failed to receive frame before timeout");
            HAL_UART_DMAStop(LIN_UART_HANDLE);
            return HAL_ERROR;
        }

        uint16_t currentCounter = __HAL_DMA_GET_COUNTER(*LIN_UART_HANDLE.hdmarx);
        uint16_t frameStart = (LIN_DMA_BUFFER_SIZE - currentCounter) - linDriver.activeFrame.length;
        uint8_t rxChecksum = getDMABufferByte(frameStart + linDriver.activeFrame.length - 1U);

        LIN_Frame_t tempFrame;
        tempFrame.id = linDriver.activeFrame.id;
        tempFrame.length = linDriver.activeFrame.length;
        tempFrame.pid = getDMABufferByte(frameStart + 1U);
        
        for(uint8_t i = 0; i < tempFrame.length; i++) {
            tempFrame.buffer[i] = getDMABufferByte(frameStart + i);
        }
        
        uint16_t sum = 0;
        if (!(tempFrame.id == 0x3C || tempFrame.id == 0x3D)) {
            sum = tempFrame.pid;
        }
        for (uint8_t i = LIN_HEADER_SIZE; i < tempFrame.length - 1U; ++i) {
            sum += tempFrame.buffer[i];
            if (sum > 0xFF) sum -= 0xFF;
        }
        
        if ((uint8_t)(~(sum & 0xFF)) != rxChecksum) {
            ERROR_PRINT("Received frame with invalid checksum");
            return HAL_ERROR;
        }

        memcpy(linDriver.activeFrame.buffer, tempFrame.buffer, tempFrame.length);
        linDriver.activeFrame.rxValid = true;
        linDriver.dmaIndex = (
            linDriver.dmaIndex + linDriver.activeFrame.length) % LIN_DMA_BUFFER_SIZE;
    }
    return HAL_OK;
}