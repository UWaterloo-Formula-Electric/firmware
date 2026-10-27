/*
    Transceiver Datasheet: https://www.ti.com/lit/ds/symlink/tlin1029-q1.pdf
*/

#include "linDriver.h"

LIN_Driver_t linDriver = {
    .settings = {
        .htim = NULL,
        .huart = NULL
    },
    .echoSize = 0,
    .dmaIndex = 0,
    .isRunning = false,
    .schedule = {
        .frameCount = 0,
        .slotCount = 0,
        .slotIndex = 0,
        .startTick = 0,
        .endTick = 0,
        .delay_us = 0
    }
};

static void stateMachine(
    LIN_Transaction_t *transaction,
    const bool cpltWithErrorNone
) {
    if (transaction == NULL) return;

    LIN_State_t nextState;

    switch (transaction->state) {
        case LIN_READY:
            if (cpltWithErrorNone) {
                nextState = LIN_TX_BREAK;
            }
            else {
                nextState = LIN_RECOVERY;
            }
            break;
        case LIN_TX_BREAK:
            if (cpltWithErrorNone) {
                nextState = LIN_TX_HEADER;
            }
            else {
                nextState = LIN_RECOVERY;
            }
            break;
        case LIN_TX_HEADER:
            if (cpltWithErrorNone) {
                nextState = LIN_TRX_RESPONSE;
            }
            else {
                nextState = LIN_RECOVERY;
            }
            break;
        case LIN_TRX_RESPONSE:
            if (cpltWithErrorNone) {
                nextState = LIN_DONE;
            }
            else {
                nextState = LIN_RECOVERY;
            }
            break;
        case LIN_RECOVERY:
            if (cpltWithErrorNone) {
                nextState = LIN_READY;
            }
            else {
                nextState = LIN_RECOVERY;
            }
            break;
        default:
            nextState = transaction->state;
            break;
    }

    transaction->state = nextState;
}

/**
 * @brief Counts the bytes written to the DMA buffer after the last read
*/
static uint16_t countNewDMABufferBytes(uint16_t writeIndex) {
    if (linDriver.dmaIndex <= writeIndex) {
        return writeIndex - linDriver.dmaIndex;
    }

    return LIN_DMA_BUFFER_SIZE - linDriver.dmaIndex + writeIndex;
}

static uint8_t getDMABufferByte(uint16_t index) {
    return linDriver.dmaBuffer[index % LIN_DMA_BUFFER_SIZE];
}

/**
 * @brief Gets the index of the first synce byte after the last read
 */
static uint16_t getFrameStartFromDMABuffer(uint16_t writeIndex) {
    uint16_t index = linDriver.dmaIndex;

    for (uint16_t n = 0; n < LIN_DMA_BUFFER_SIZE; ++n) {
        if (getDMABufferByte(index) == LIN_SYNC_BYTE) {
            return index % LIN_DMA_BUFFER_SIZE;
        }

        ++index;
    }

    return UINT16_MAX;
}

HAL_StatusTypeDef LIN_setSettings(LIN_Settings_t *settings) {
    if (settings == NULL ||
        settings->htim == NULL ||
        settings->huart == NULL) {
        return HAL_ERROR;
    }

    linDriver.settings.htim = settings->htim;
    linDriver.settings.huart = settings->huart;

    return HAL_OK;
}

HAL_StatusTypeDef LIN_setSchedule(LIN_Schedule_t *schedule) {
    if (linDriver.isRunning || schedule == NULL) {
        return HAL_ERROR;
    }

    memcpy(linDriver.schedule.frames, schedule->frames, schedule->frameCount);
    memcpy(linDriver.schedule.slots, schedule->slots, schedule->slotCount);

    linDriver.schedule.frameCount = schedule->frameCount;
    linDriver.schedule.slotCount = schedule->slotCount;
    linDriver.schedule.slotIndex = 0;
    linDriver.schedule.startTick = 0;
    linDriver.schedule.endTick = 0;
    linDriver.schedule.delay_us = 0;

    return HAL_OK;
}

HAL_StatusTypeDef LIN_setProtectedID(LIN_Frame_t *frame) {
    if (frame == NULL || frame->id > LIN_MAX_FRAME_ID) return HAL_ERROR;

    uint8_t id = frame->id;
    uint8_t po = (id ^ (id >> 1U) ^ (id >> 2U) ^ (id >> 4U)) & 1U;
    uint8_t p1 = (~((id >> 1) ^ (id >> 3U) ^ (id >> 4) ^ (id >> 5))) & 1U;

    frame->pid = id | (po << 6U) | (p1 << 7U);

    return HAL_OK;
}

HAL_StatusTypeDef LIN_setChecksum(LIN_Frame_t *frame) {
    if (frame == NULL || frame->responseSize == 0) return HAL_ERROR;

    uint16_t sum = 0;

    if (frame->checksumType == LIN_CHECKSUM_ENHANCED) {
        sum = frame->pid;
    }

    for (uint8_t i = 0; i < frame->responseSize - 1; ++i) {
        sum += frame->responseBytes[i];

        if (sum > 0xFF) {
            sum -= 0xFF;
        }
    }

    frame->responseBytes[frame->responseSize] = ~(sum & 0xFF);

    return HAL_OK;
}

HAL_StatusTypeDef LIN_setChecksumType(LIN_Frame_t *frame) {
    if (frame == NULL || frame->id > LIN_MAX_FRAME_ID) return HAL_ERROR;

    if (frame->id == LIN_DIAG_MASTER_REQUEST_FRAME ||
        frame->id == LIN_DIAG_SLAVE_RESPONSE_FRAME) {
        frame->checksumType = LIN_CHECKSUM_CLASSIC;
    }
    else {
        frame->checksumType = LIN_CHECKSUM_ENHANCED;
    }

    return HAL_OK;
}

HAL_StatusTypeDef LIN_deepcopyFrame(const LIN_Frame_t *original, LIN_Frame_t *copy) {
    if (original == NULL || copy == NULL) return HAL_ERROR;

    copy->id = original->id;
    copy->pid = original->pid;
    copy->checksumType = original->checksumType;
    copy->direction = original->direction;
    copy->responseSize = original->responseSize;

    memcpy(copy->headerBytes, original->headerBytes, LIN_HEADER_SIZE);
    memcpy(copy->responseBytes, original->responseBytes, copy->responseSize);

    copy->rxValidResponse = original->rxValidResponse;
    copy->txUpdatePending = original->txUpdatePending;

    if (copy->txUpdatePending) {
        copy->updateSize = original->updateSize;
        memcpy(copy->updateBuffer, original->updateBuffer, copy->updateSize);
    }

    return HAL_OK;
}

HAL_StatusTypeDef LIN_updateResponse(
    uint8_t *data,
    uint8_t size,
    uint8_t index) {
    if (data == NULL || size == 0 || index > LIN_MAX_FRAMES) {
        return HAL_ERROR;
    }

    LIN_Frame_t *p_frame = &linDriver.schedule.frames[index];

    if (p_frame->direction == LIN_MASTER_SUBSCRIBE) return HAL_ERROR;

    memcpy(p_frame->updateBuffer, data, size);
    p_frame->updateSize = size;
    p_frame->txUpdatePending = true;

    return HAL_OK;
}

HAL_StatusTypeDef LIN_startSchedule(void) {
    if (linDriver.isRunning) return HAL_ERROR;

    linDriver.isRunning = true;
    linDriver.schedule.slotIndex  = 0;
    linDriver.schedule.startTick = __HAL_TIM_GET_COUNTER(linDriver.settings.htim);

    return HAL_LIN_SendBreak(linDriver.settings.huart);
}

static void goToNextSlot(void) {
    uint8_t nextIndex = (linDriver.schedule.slotIndex + 1) % linDriver.schedule.slotCount;
    linDriver.schedule.slotIndex = nextIndex;

    LIN_Slot_t *p_nextSlot = &linDriver.schedule.slots[nextIndex];
    LIN_Frame_t *p_nextFrame = &linDriver.schedule.frames[p_nextSlot->frameIndex];
    linDriver.transaction.direction = p_nextFrame->direction;
}

void LIN_HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim == NULL || htim != linDriver.settings.htim) return;

    linDriver.schedule.endTick = __HAL_TIM_GET_COUNTER(linDriver.settings.htim);
    uint32_t elapsed_us = linDriver.schedule.endTick - linDriver.schedule.startTick;

    LIN_Slot_t *p_slot = &linDriver.schedule.slots[linDriver.schedule.slotIndex];
    LIN_Transaction_t *p_transaction = &linDriver.transaction;

    if (elapsed_us >= p_slot->duration_us && linDriver.schedule.delay_us == 0) {
        if(p_transaction->error == LIN_ERROR_NONE) {
            p_transaction->error = LIN_TIMEOUT;
        }
    }

    bool startSlot = false;
    
    if (p_transaction->state == LIN_DONE && p_transaction->error == LIN_ERROR_NONE) {
        p_transaction->state = LIN_READY;
        startSlot = true;
        goToNextSlot();
    }
    
    if (p_transaction->error != LIN_ERROR_NONE || p_transaction->state == LIN_RECOVERY) {
        p_transaction->state = LIN_READY;
        p_transaction->error = LIN_ERROR_NONE;

        HAL_UART_DMAStop(linDriver.settings.huart);
        memset(linDriver.dmaBuffer, 0, LIN_DMA_BUFFER_SIZE);
        linDriver.dmaIndex = 0;

        if (p_transaction->tryCount < p_slot->maxTries) {
            ++p_transaction->tryCount;
        } else {
            goToNextSlot();
        }

        elapsed_us = 0;
        uint32_t tBit_us = 1 / linDriver.settings.huart->Init.BaudRate;
        uint32_t numBits = LIN_BITS_PER_BYTE * (LIN_HEADER_SIZE + LIN_MAX_RESPONSE_SIZE);
        linDriver.schedule.delay_us = tBit_us * numBits;
        linDriver.schedule.startTick = __HAL_TIM_GET_COUNTER(linDriver.settings.htim);
    }

    if (elapsed_us >= linDriver.schedule.delay_us) {
        linDriver.schedule.delay_us = 0;
        startSlot = true;

        HAL_UARTEx_ReceiveToIdle_DMA(
            linDriver.settings.huart,
            linDriver.dmaBuffer,
            LIN_DMA_BUFFER_SIZE);
    }

    if (startSlot) {
        p_transaction->state = LIN_TX_BREAK;
        linDriver.schedule.startTick = __HAL_TIM_GET_COUNTER(linDriver.settings.htim);
        
        HAL_StatusTypeDef txStatus = HAL_LIN_SendBreak(linDriver.settings.huart);

        if (txStatus == HAL_BUSY) {
            p_transaction->error = LIN_BIT_ERROR;
        }
        else if (txStatus == HAL_ERROR) {
            p_transaction->error = LIN_UART_ERROR;
        }
    }
}

static void readDMABuffer(void) {
    LIN_Transaction_t *p_transaction = &linDriver.transaction;
    LIN_Slot_t *p_slot = &linDriver.schedule.slots[linDriver.schedule.slotIndex];
    LIN_Frame_t *p_frame = &linDriver.schedule.frames[p_slot->frameIndex];

    bool cpltWithErrorNone = true;

    switch (p_transaction->state) {
        case LIN_TX_HEADER:
            if (linDriver.echoSize < LIN_HEADER_SIZE) return;

            cpltWithErrorNone = linDriver.echoBytes[0] == LIN_SYNC_BYTE &&
                                linDriver.echoBytes[1] == p_frame->pid;

            if (!cpltWithErrorNone) {
                p_transaction->error = LIN_BIT_ERROR;
            }
            break;
        case LIN_TRX_RESPONSE:
            if (linDriver.echoSize < p_frame->responseSize + LIN_HEADER_SIZE) return;

            if (p_frame->direction == LIN_MASTER_PUBLISH) {
                for (uint8_t i = 0; i < p_frame->responseSize; ++i) {
                    if (linDriver.echoBytes[2 + i] != p_frame->responseBytes[i]) {
                        cpltWithErrorNone = false;
                        p_transaction->error = LIN_BIT_ERROR;
                        break;
                    }
                }
            }
            else {
                LIN_Frame_t tempFrame;

                LIN_deepcopyFrame(p_frame, &tempFrame);
                memcpy(
                    tempFrame.responseBytes,
                    linDriver.echoBytes + LIN_HEADER_SIZE,
                    tempFrame.responseSize - 1);

                HAL_StatusTypeDef calcStatus = LIN_setChecksum(&tempFrame);

                if (calcStatus == HAL_OK) {
                    uint8_t expectedChecksum = tempFrame.responseBytes[tempFrame.responseSize];
                    uint8_t rxChecksum = linDriver.echoBytes[p_frame->responseSize + LIN_HEADER_SIZE];

                    if (expectedChecksum == rxChecksum) {
                        p_frame->rxValidResponse = true;
                        memcpy(
                            p_frame->responseBytes,
                            tempFrame.responseBytes,
                            tempFrame.responseSize);
                    }
                    else {
                        p_frame->rxValidResponse = false;
                        cpltWithErrorNone = false;
                        p_transaction->error = LIN_CHECKSUM_ERROR;
                    }
                }
                else {
                    p_frame->rxValidResponse = false;
                    cpltWithErrorNone = false;
                    p_transaction->error = LIN_BAD_PARAMS;
                } 
            }
            break;
        default:
            return;
    }

    stateMachine(p_transaction, cpltWithErrorNone);

    if (cpltWithErrorNone &&
        p_transaction->state == LIN_TRX_RESPONSE &&
        p_frame->direction == LIN_MASTER_PUBLISH) {
        if (p_frame->txUpdatePending) {
            p_frame->txUpdatePending = false;
            p_frame->responseSize = p_frame->updateSize;
            memcpy(p_frame->responseBytes, p_frame->updateBuffer, p_frame->responseSize - 1);

            HAL_StatusTypeDef calcStatus = LIN_setChecksum(p_frame);

            if (calcStatus == HAL_OK) {
                HAL_StatusTypeDef txStatus = HAL_UART_Transmit_DMA(
                    linDriver.settings.huart,
                    p_frame->responseBytes,
                    p_frame->responseSize);
                    
                if (txStatus == HAL_BUSY) {
                    p_transaction->error = LIN_BIT_ERROR;
                }
                else if (txStatus == HAL_ERROR) {
                    p_transaction->error = LIN_DMA_ERROR;
                }
            }
            else {
                p_transaction->error = LIN_BAD_PARAMS;
            }
        }
    }
}

void LIN_HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart == NULL || huart != linDriver.settings.huart) return;

    uint16_t numRxBytes = countNewDMABufferBytes(Size);
    if (numRxBytes == 0) return;

    LIN_Transaction_t *p_transaction = &linDriver.transaction;
    if (p_transaction->state == LIN_READY) {
        // Discard junk bytes
        linDriver.dmaIndex = Size % LIN_DMA_BUFFER_SIZE;
        return;
    }

    // For reading a new frame
    if (linDriver.echoSize == 0) {
        uint16_t startIndex = getFrameStartFromDMABuffer(Size);

        // No sync byte found in new bytes
        if (startIndex == UINT16_MAX) {
            // Discard junk bytes
            linDriver.dmaIndex = Size % LIN_DMA_BUFFER_SIZE;
            return;
        }
        
        linDriver.dmaIndex = startIndex;
        
        // Count new bytes from new start
        numRxBytes = countNewDMABufferBytes(Size);
    }

    LIN_Slot_t *p_slot = &linDriver.schedule.slots[linDriver.schedule.slotIndex];
    LIN_Frame_t *p_frame = &linDriver.schedule.frames[p_slot->frameIndex];

    uint16_t numExpectedBytes = 0;
    
    if (p_transaction->state == LIN_TX_HEADER) {
        numExpectedBytes = LIN_HEADER_SIZE;
    }
    else if (p_transaction->state == LIN_TRX_RESPONSE) {
        numExpectedBytes = p_frame->responseSize;
    }

    uint16_t numBytesLeft = (p_frame->responseSize + LIN_HEADER_SIZE) - linDriver.echoSize;

    uint16_t numBytesToRead = 0;

    if (numExpectedBytes < numRxBytes) {
        numBytesToRead = numExpectedBytes;
    }
    else if (numBytesLeft < numRxBytes) {
        numBytesToRead = numBytesLeft;
    }
    else {
        numBytesToRead = numRxBytes;
    }

    for (uint16_t i = 0; i < numBytesToRead; ++i) {
        linDriver.echoBytes[i + linDriver.echoSize] = getDMABufferByte(linDriver.dmaIndex + i);
    }

    linDriver.echoSize += numBytesToRead;
    linDriver.dmaIndex = (linDriver.dmaIndex + numBytesToRead) % LIN_DMA_BUFFER_SIZE;

    readDMABuffer();
}

void LIN_HAL_UART_IRQHandler(UART_HandleTypeDef *huart) {
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_LBDF) != RESET) {
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_LBDF);
        // UART detects break as a framing error
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_FE);

        LIN_Transaction_t *p_transaction = &linDriver.transaction;

        if (p_transaction->state == LIN_TX_BREAK) {
            stateMachine(p_transaction, true);

            LIN_Slot_t *p_slot = &linDriver.schedule.slots[linDriver.schedule.slotIndex];
            LIN_Frame_t *p_frame = &linDriver.schedule.frames[p_slot->frameIndex];

            p_frame->headerBytes[0] = LIN_SYNC_BYTE;
            p_frame->headerBytes[1] = p_frame->pid;

            HAL_StatusTypeDef txStatus = HAL_UART_Transmit_DMA(
                linDriver.settings.huart,
                p_frame->headerBytes, 2);
            
            if (txStatus == HAL_BUSY) {
                p_transaction->error = LIN_BIT_ERROR;
            }
            else if (txStatus == HAL_ERROR) {
                p_transaction->error = LIN_DMA_ERROR;
            }
        }
    }
    HAL_UART_IRQHandler(huart);
}

void LIN_HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart == NULL || huart != linDriver.settings.huart) return;

    switch (linDriver.settings.huart->ErrorCode) {
        case HAL_UART_ERROR_ORE:
            linDriver.transaction.error = LIN_OVERRUN_ERROR;
            break;
        case HAL_UART_ERROR_FE:
            linDriver.transaction.error = LIN_FRAMING_ERROR;
            break;
        case HAL_UART_ERROR_NE:
            linDriver.transaction.error = LIN_NOISE_ERROR;
            break;
        default:
            linDriver.transaction.error = LIN_UART_ERROR;
            break;
    }
}