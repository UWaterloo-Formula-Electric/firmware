/*
    Transceiver Datasheet: https://www.ti.com/lit/ds/symlink/tlin1029-q1.pdf
*/

#include "linDriver.h"

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

static uint8_t getDMABufferByte(LIN_Driver_t *driver, uint16_t index) {
    return driver->dmaBuffer[index % LIN_DMA_BUFFER_SIZE];
}

static uint16_t countNewDMABufferBytes(LIN_Driver_t *driver, uint16_t writeIndex) {
    if (driver->dmaIndex <= writeIndex) {
        return writeIndex - driver->dmaIndex;
    }

    return LIN_DMA_BUFFER_SIZE - driver->dmaIndex + writeIndex;
}

static uint16_t getFrameStartFromDMABuffer(LIN_Driver_t *driver, uint16_t writeIndex) {
    uint16_t index = driver->dmaIndex;
    uint16_t numRxBytes = countNewDMABufferBytes(driver, writeIndex);

    for (uint16_t n = 0; n < numRxBytes; ++n) {
        if (getDMABufferByte(driver, index) == LIN_SYNC_BYTE) {
            return index % LIN_DMA_BUFFER_SIZE;
        }

        ++index;
    }

    return UINT16_MAX;
}

static void readDMABuffer(LIN_Driver_t *driver) {
    if (driver == NULL) return;

    LIN_Transaction_t *txn = &(driver->transaction);
    LIN_Slot_t *slot = &driver->schedule.slots[driver->schedule.slotIndex];
    LIN_Frame_t *frame = &driver->schedule.frames[slot->frameIndex];

    bool cpltWithErrorNone = true;

    switch (txn->state) {
        case LIN_TX_HEADER:
            if (driver->echoSize < LIN_HEADER_SIZE) return;

            cpltWithErrorNone = driver->echoBytes[0] == LIN_SYNC_BYTE &&
                                driver->echoBytes[1] == frame->pid;

            if (!cpltWithErrorNone) {
                txn->error = LIN_BIT_ERROR;
            }

            break;
        case LIN_TRX_RESPONSE:
            if (driver->echoSize < frame->responseSize + LIN_HEADER_SIZE) return;

            if (frame->direction == LIN_MASTER_PUBLISH) {
                for (uint8_t i = 0; i < frame->responseSize; ++i) {
                    if (driver->echoBytes[2 + i] != frame->responseBytes[i]) {
                        cpltWithErrorNone = false;
                        txn->error = LIN_BIT_ERROR;
                        break;
                    }
                }
            }
            else {
                LIN_Frame_t tempFrame;

                LIN_deepcopyFrame(frame, &tempFrame);
                memcpy(
                    tempFrame.responseBytes,
                    driver->echoBytes + LIN_HEADER_SIZE,
                    tempFrame.responseSize - 1);

                HAL_StatusTypeDef calcStatus = LIN_setChecksum(&tempFrame);

                if (calcStatus == HAL_OK) {
                    uint8_t expectedChecksum = tempFrame.responseBytes[tempFrame.responseSize];
                    uint8_t rxChecksum = driver->echoBytes[frame->responseSize + LIN_HEADER_SIZE];

                    if (expectedChecksum == rxChecksum) {
                        frame->rxValidResponse = true;
                        memcpy(
                            frame->responseBytes,
                            tempFrame.responseBytes,
                            tempFrame.responseSize);
                    }
                    else {
                        frame->rxValidResponse = false;
                        cpltWithErrorNone = false;
                        txn->error = LIN_CHECKSUM_ERROR;
                    }
                }
                else {
                    frame->rxValidResponse = false;
                    cpltWithErrorNone = false;
                    txn->error = LIN_BAD_PARAMS;
                } 
            }
            break;
        default:
            return;
    }

    stateMachine(txn, cpltWithErrorNone);

    if (cpltWithErrorNone &&
        txn->state == LIN_TRX_RESPONSE &&
        frame->direction == LIN_MASTER_PUBLISH) {
        if (frame->txUpdatePending) {
            frame->txUpdatePending = false;
            frame->responseSize = frame->updateSize;
            memcpy(frame->responseBytes, frame->updateBuffer, frame->responseSize - 1);

            HAL_StatusTypeDef calcStatus = LIN_setChecksum(frame);

            if (calcStatus == HAL_OK) {
                HAL_StatusTypeDef txStatus = HAL_UART_Transmit_DMA(
                    driver->huart,
                    frame->responseBytes,
                    frame->responseSize);
                    
                if (txStatus == HAL_BUSY) {
                    txn->error = LIN_BIT_ERROR;
                }
                else if (txStatus == HAL_ERROR) {
                    txn->error = LIN_DMA_ERROR;
                }
            }
            else {
                txn->error = LIN_BAD_PARAMS;
            }
        }
    }
}

static void goToNextSlot(LIN_Driver_t *driver) {
    uint8_t nextIndex = (driver->schedule.slotIndex + 1) % driver->schedule.slotCount;
    driver->schedule.slotIndex = nextIndex;

    LIN_Slot_t *nextSlot = &driver->schedule.slots[nextIndex];
    LIN_Frame_t *nextFrame = &driver->schedule.frames[nextSlot->frameIndex];
    driver->transaction.direction = nextFrame->direction;
}

HAL_StatusTypeDef LIN_frameInitDefault(LIN_Frame_t *frame) {
    if (frame == NULL) return HAL_ERROR;

    frame->responseSize = 0;
    frame->updateSize = 0;
    frame->rxValidResponse = false;
    frame->txUpdatePending = false;

    memset(frame->headerBytes, 0, 2);
    memset(frame->responseBytes, 0, LIN_MAX_DATA_SIZE + 1);
    memset(frame->updateBuffer, 0, LIN_MAX_DATA_SIZE);

    return HAL_OK;
}

HAL_StatusTypeDef LIN_scheduleInitDefault(LIN_Schedule_t *schedule) {
    if (schedule == NULL) return HAL_ERROR;

    schedule->frameCount = 0;
    schedule->slotCount = 0;
    schedule->slotIndex = 0;
    schedule->startTick = 0;
    schedule->endTick = 0;

    memset(schedule->frames, 0, LIN_MAX_FRAMES * sizeof(LIN_Frame_t));
    memset(schedule->slots, 0, LIN_MAX_SLOTS * sizeof(LIN_Slot_t));

    return HAL_OK;
}

HAL_StatusTypeDef LIN_driverInitDefault(LIN_Driver_t *driver) {
    if (driver == NULL) return HAL_ERROR;

    driver->huart = NULL;
    driver->htim = NULL;
    driver->echoSize = 0;
    driver->dmaIndex = 0;
    driver->isRunning = false;

    memset(driver->dmaBuffer, 0, LIN_DMA_BUFFER_SIZE);
    memset(driver->echoBytes, 0, LIN_MAX_DATA_SIZE + 3);

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
    LIN_Schedule_t *schedule,
    uint8_t *data,
    uint8_t size,
    uint8_t index) {
    if (schedule == NULL || data == NULL ||
        size == 0 || index > LIN_MAX_FRAMES) {
        return HAL_ERROR;
    }

    LIN_Frame_t *p_frame = &schedule->frames[index];

    if (p_frame->direction == LIN_MASTER_SUBSCRIBE) return HAL_ERROR;

    memcpy(p_frame->updateBuffer, data, size);
    p_frame->updateSize = size;
    p_frame->txUpdatePending = true;

    return HAL_OK;
}

HAL_StatusTypeDef LIN_startSchedule(LIN_Driver_t *driver) {
    if (driver == NULL || driver->isRunning ||
        driver->huart == NULL || driver->htim == NULL) {
        return HAL_ERROR;
    }

    driver->isRunning = true;
    driver->schedule.slotIndex  = 0;
    driver->schedule.startTick = __HAL_TIM_GET_COUNTER(driver->htim);

    return HAL_LIN_SendBreak(driver->huart);
}

void LIN_HAL_TIM_PeriodElapsedCallback(
    LIN_Driver_t *driver,
    TIM_HandleTypeDef *htim) {
    if (htim == NULL || driver == NULL || htim != driver->htim) return;

    driver->schedule.endTick = __HAL_TIM_GET_COUNTER(driver->htim);
    uint32_t elapsed_us = driver->schedule.endTick - driver->schedule.startTick;

    LIN_Slot_t *slot = &driver->schedule.slots[driver->schedule.slotIndex];
    LIN_Transaction_t *txn = &driver->transaction;

    if (elapsed_us >= slot->duration_us) {
        if(txn->error == LIN_ERROR_NONE) {
            txn->error = LIN_TIMEOUT;
        }
    }

    bool startSlot = false;
    
    if (txn->state == LIN_DONE && txn->error == LIN_ERROR_NONE) {
        startSlot = true;
        goToNextSlot(driver);
    }
    
    if (txn->error != LIN_ERROR_NONE) {
        startSlot = true;

        HAL_UART_DMAStop(driver->huart);

        memset(driver->dmaBuffer, 0, LIN_DMA_BUFFER_SIZE);
        driver->dmaIndex = 0;

        HAL_UARTEx_ReceiveToIdle_DMA(driver->huart, driver->dmaBuffer, LIN_DMA_BUFFER_SIZE);

        if (txn->tryCount < slot->maxTries) {
            ++txn->tryCount;
        } else {
            goToNextSlot(driver);
        }
    }

    if (startSlot) {
        txn->state = LIN_TX_BREAK;
        driver->schedule.endTick = 0;
        driver->schedule.startTick = __HAL_TIM_GET_COUNTER(driver->htim);
        
        HAL_StatusTypeDef txStatus = HAL_LIN_SendBreak(driver->huart);

        if (txStatus == HAL_BUSY) {
            txn->error = LIN_BIT_ERROR;
        }
        else if (txStatus == HAL_ERROR) {
            txn->error = LIN_UART_ERROR;
        }
    }
}

void LIN_HAL_UARTEx_RxEventCallback(
    LIN_Driver_t *driver,
    UART_HandleTypeDef *huart,
    uint16_t Size) {
    if (driver == NULL || huart == NULL ||
        huart != driver->huart) {
        return;
    }

    uint16_t numRxBytes = countNewDMABufferBytes(driver, Size);
    if (numRxBytes == 0) return;

    LIN_Transaction_t *txn = &(driver->transaction);
    if (txn->state == LIN_READY) {
        // Discard junk bytes
        driver->dmaIndex = Size % LIN_DMA_BUFFER_SIZE;
        return;
    }

    // For reading a new frame
    if (driver->echoSize == 0) {
        uint16_t startIndex = getFrameStartFromDMABuffer(driver, Size);

        // No sync byte found in new bytes
        if (startIndex == UINT16_MAX) {
            // Discard junk bytes
            driver->dmaIndex = Size % LIN_DMA_BUFFER_SIZE;
            return;
        }
        
        driver->dmaIndex = startIndex;
        
        // Count new bytes from new start
        numRxBytes = countNewDMABufferBytes(driver, Size);
    }

    LIN_Slot_t *slot = &driver->schedule.slots[driver->schedule.slotIndex];
    LIN_Frame_t *frame = &driver->schedule.frames[slot->frameIndex];

    uint16_t numExpectedBytes = 0;
    
    if (txn->state == LIN_TX_HEADER) {
        numExpectedBytes = LIN_HEADER_SIZE;
    }
    else if (txn->state == LIN_TRX_RESPONSE) {
        numExpectedBytes = frame->responseSize;
    }

    uint16_t numBytesLeft = (frame->responseSize + LIN_HEADER_SIZE) - driver->echoSize;

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
        driver->echoBytes[i + driver->echoSize] = getDMABufferByte(
            driver, driver->dmaIndex + i);
    }

    driver->echoSize += numBytesToRead;
    driver->dmaIndex = (driver->dmaIndex + numBytesToRead) % LIN_DMA_BUFFER_SIZE;

    readDMABuffer(driver);
}

void LIN_HAL_UART_IRQHandler(
    LIN_Driver_t *driver,
    UART_HandleTypeDef *huart) {
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_LBDF) != RESET) {
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_LBDF);
        // UART detects break as a framing error
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_FE);

        LIN_Transaction_t *txn = &(driver->transaction);

        if (txn->state == LIN_TX_BREAK) {
            stateMachine(txn, true);

            LIN_Slot_t *slot = &(driver->schedule.slots[driver->schedule.slotIndex]);
            LIN_Frame_t *frame = &(driver->schedule.frames[slot->frameIndex]);

            frame->headerBytes[0] = LIN_SYNC_BYTE;
            frame->headerBytes[1] = frame->pid;

            HAL_StatusTypeDef txStatus = HAL_UART_Transmit_DMA(huart, frame->headerBytes, 2);
            
            if (txStatus == HAL_BUSY) {
                txn->error = LIN_BIT_ERROR;
            }
            else if (txStatus == HAL_ERROR) {
                txn->error = LIN_DMA_ERROR;
            }
        }
    }
    HAL_UART_IRQHandler(huart);
}

void LIN_HAL_UART_ErrorCallback(
    LIN_Driver_t *driver,
    UART_HandleTypeDef *huart) {
    if (driver == NULL || huart == NULL ||
        huart != driver->huart) {
        return;
    }

    switch (driver->huart->ErrorCode) {
        case HAL_UART_ERROR_ORE:
            driver->transaction.error = LIN_OVERRUN_ERROR;
            break;
        case HAL_UART_ERROR_FE:
            driver->transaction.error = LIN_FRAMING_ERROR;
            break;
        case HAL_UART_ERROR_NE:
            driver->transaction.error = LIN_NOISE_ERROR;
            break;
        default:
            driver->transaction.error = LIN_UART_ERROR;
            break;
    }
}