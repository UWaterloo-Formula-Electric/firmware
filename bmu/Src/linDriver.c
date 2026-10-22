#include "linDriver.h"

HAL_StatusTypeDef LIN_init_frame(
    LIN_Frame_t *frame,
    uint8_t id,
    uint8_t length,
    LIN_Direction_t direction,
    LIN_FrameType_t frame_type) {
    if (frame == NULL || id > LIN_MAX_FRAME_ID ||
        length == 0 || length > LIN_MAX_DATA_SIZE) {
        return HAL_ERROR;
    }

    frame->id = id;
    
    LIN_make_protected_ID(frame->id, &(frame->pid));
    
    frame->length = length;
    frame->direction = direction;
    frame->frame_type = frame_type;
    // LIN 2.x diagnostic frames use classic checksum
    if (frame->frame_type == LIN_FRAME_DIAGNOSTIC) {
        frame->checksum_type = LIN_CHECKSUM_CLASSIC;
    }
    else {
        frame->checksum_type = LIN_CHECKSUM_ENHANCED;
    }

    frame->rx_valid_response = false;
    frame->tx_update_pending = false;

    memset(frame->header_bytes, 0, 2);
    memset(frame->response_bytes, 0, LIN_MAX_DATA_SIZE + 1);
    memset(frame->update_buffer, 0, LIN_MAX_DATA_SIZE);

    return HAL_OK;
}

HAL_StatusTypeDef LIN_init_schedule(
    LIN_Schedule_t *schedule,
    LIN_Frame_t *frames,
    uint8_t frame_count,
    LIN_Slot_t *slots,
    uint8_t slot_count) {
    if (schedule == NULL || frames == NULL || slots == NULL ||
        frame_count == 0 || frame_count > LIN_MAX_FRAMES ||
        slot_count == 0 || slot_count > LIN_MAX_SLOTS) {
        return HAL_ERROR;
    }

    memcpy(schedule->frames, frames,
       frame_count * sizeof(frames[0]));
    memcpy(schedule->slots, slots,
        slot_count * sizeof(slots[0]));

    schedule->frame_count = frame_count;
    schedule->slot_count = slot_count;
    schedule->slot_index = 0;
    schedule->elapsed_us = 0;

    return HAL_OK;
}

HAL_StatusTypeDef LIN_init_driver(
    LIN_Driver_t *driver,
    LIN_Schedule_t *schedule,
    TIM_HandleTypeDef *htim,
    UART_HandleTypeDef *huart
) {
    if (driver == NULL || schedule == NULL ||
        huart == NULL || htim == NULL ||
        schedule->frame_count == 0 ||
        schedule->frame_count > LIN_MAX_FRAMES ||
        schedule->slot_count == 0 ||
        schedule->slot_count > LIN_MAX_SLOTS) {
        return HAL_ERROR;
        }

    // Validate baud rate range
    if (huart->Init.BaudRate < LIN_MIN_BAUD || huart->Init.BaudRate > LIN_MAX_BAUD) {
        return HAL_ERROR;
    }

    // Validate word length
    if (huart->Init.WordLength != UART_WORDLENGTH_8B) {
        return HAL_ERROR;
    }

    // Ensure that LIN mode is enabled
    if ((huart->Instance->CR2 & USART_CR2_LINEN) == 0U) {
        return HAL_ERROR;
    }

    uint32_t baudrate = huart->Init.BaudRate;

    for (uint8_t i = 0; i < schedule->slot_count; ++i) {
        LIN_Slot_t *slot = &(schedule->slots[i]);
        
        if (slot->frame_index > schedule->frame_count) {
            return HAL_ERROR;
        }

        LIN_Frame_t *frame = &(schedule->frames[slot->frame_index]);

        if (frame->length > LIN_MAX_DATA_SIZE ||
            frame->id > LIN_MAX_FRAME_ID) {
            return HAL_ERROR;
        }

        uint8_t expected_pid = 0;

        LIN_make_protected_ID(frame->id, &expected_pid);

        if (frame->pid != expected_pid) {
            return HAL_ERROR;
        }

        uint32_t min_duration_us = LIN_calculate_max_t_transmission_us(
            baudrate,
            LIN_HEADER_BITS + (frame->length + 1) * LIN_BITS_PER_BYTE);

        min_duration_us = LIN_round_t_tranmission_to_timebase_us(min_duration_us);

        if (slot->duration_us < min_duration_us) {
            return HAL_ERROR;
        }
    }

    memset(driver, 0, sizeof(LIN_Driver_t));
    driver->schedule = *schedule;
    driver->htim = htim;
    driver->huart = huart;
    // Arm circular RX DMA
    HAL_StatusTypeDef circular_dma_status = HAL_UARTEx_ReceiveToIdle_DMA(
        driver->huart,
        driver->dma_buffer,
        LIN_DMA_BUFFER_SIZE
    );
    
    if (circular_dma_status != HAL_OK) {
        return circular_dma_status;
    }

    // Enable LIN Break Detection (LBD) interrupt
    __HAL_UART_ENABLE_IT(huart, UART_IT_LBD);

    return HAL_OK;
}

HAL_StatusTypeDef LIN_make_protected_ID(uint8_t id, uint8_t *p_id) {
    if (p_id == NULL || id > LIN_MAX_FRAME_ID) return HAL_ERROR;

    uint8_t po = (id ^ (id >> 1U) ^ (id >> 2U) ^ (id >> 4U)) & 1U;
    uint8_t p1 = (~((id >> 1) ^ (id >> 3U) ^ (id >> 4) ^ (id >> 5))) & 1U;

    *p_id = id | (po << 6U) | (p1 << 7U);

    return HAL_OK;
}

HAL_StatusTypeDef LIN_calculate_checksum(
    LIN_ChecksumType_t checksum_type,
    const uint8_t pid,
    const uint8_t length,
    const uint8_t *bytes,
    uint8_t *p_checksum) {
    if (p_checksum == NULL || bytes == NULL || length == 0) return HAL_ERROR;

    uint16_t sum = 0;

    if (checksum_type == LIN_CHECKSUM_ENHANCED) {
        sum = pid;
    }

    for (uint8_t i = 0; i < length; ++i) {
        sum += bytes[i];

        if (sum > 0xFF) {
            sum -= 0xFF;
        }
    }

    *p_checksum = ~(sum & 0xFF);

    return HAL_OK;
}

HAL_StatusTypeDef LIN_update_frame(
    LIN_Schedule_t *schedule,
    uint8_t index,
    uint8_t *data,
    uint8_t length) {
    if (schedule == NULL || index >= schedule->frame_count ||
        data == NULL || length == 0 || length > LIN_MAX_DATA_SIZE) {
        return HAL_ERROR;
    }

    LIN_Frame_t *frame = &schedule->frames[index];

    if (frame->direction == LIN_MASTER_SUBSCRIBE ||
        length == 0U ||
        length > LIN_MAX_DATA_SIZE) {
        return HAL_ERROR;
    }
    
    frame->tx_update_pending = true;
    memcpy(frame->update_buffer, data, length);
    frame->length = length;

    return HAL_OK;
}

uint32_t LIN_calculate_nominal_t_transmission_us(
    const uint32_t baudrate,
    const uint8_t length) {
    uint32_t t_bit_us = 1e6 / baudrate;
    return length * t_bit_us;
}

uint32_t LIN_calculate_max_t_transmission_us(
    const uint32_t baudrate,
    const uint8_t length) {
    uint32_t t_trx_us = LIN_calculate_nominal_t_transmission_us(baudrate, length);
    return (t_trx_us * LIN_TIME_BUFFER_PCT + 50) / 100;
}

uint32_t LIN_round_t_tranmission_to_timebase_us(const uint32_t t_us) {
    uint32_t rem = t_us % LIN_TIMEBASE_US;

    if (rem == 0) {
        return t_us;
    }

    return t_us + (LIN_TIMEBASE_US - rem);
}

void LIN_master_state_machine(
    LIN_Transaction_t *transaction,
    const bool is_complete
) {
    if (transaction == NULL) return;

    LIN_MasterState_t next_state;

    // LIN 2.x signal transmitted at last time to write
    switch (transaction->master_state) {
        case LIN_MASTER_IDLE:
            if (is_complete) {
                next_state = LIN_MASTER_TX_BREAK;
            }
            else {
                next_state = LIN_MASTER_ERROR;
            }
            break;
        case LIN_MASTER_TX_BREAK:
            if (is_complete) {
                next_state = LIN_MASTER_TX_HEADER;
            }
            else {
                next_state = LIN_MASTER_ERROR;
            }
            break;
        case LIN_MASTER_TX_HEADER:
            if (is_complete) {
                if (transaction->direction == LIN_MASTER_PUBLISH) {
                    next_state = LIN_MASTER_TX_RESPONSE;
                }
                else {
                    next_state = LIN_MASTER_RX_RESPONSE;
                }
            }
            else {
                next_state = LIN_MASTER_ERROR;
            }
            break;
        case LIN_MASTER_TX_RESPONSE:
            if (is_complete) {
                next_state = LIN_MASTER_SUCCESS;
            }
            else {
                next_state = LIN_MASTER_ERROR;
            }
            break;
        case LIN_MASTER_RX_RESPONSE:
            if (is_complete) {
                next_state = LIN_MASTER_SUCCESS;
            }
            else {
                next_state = LIN_MASTER_ERROR;
            }
            break;
        case LIN_MASTER_ERROR:
            if (is_complete) {
                next_state = LIN_MASTER_IDLE;
            }
            else {
                next_state = LIN_MASTER_ERROR;
            }
            break;
        default:
            next_state = transaction->master_state;
            break;
    }

    transaction->master_state = next_state;
}

void LIN_read_dma_buffer(LIN_Driver_t *driver) {
    if (driver == NULL) return;

    LIN_Transaction_t *txn = &(driver->transaction);
    LIN_Slot_t *slot = &(driver->schedule.slots[driver->schedule.slot_index]);
    LIN_Frame_t *frame = &(driver->schedule.frames[slot->frame_index]);

    bool is_complete = true;

    switch (txn->master_state) {
        case LIN_MASTER_TX_HEADER:
            if (driver->echo_size < 2) return;

            is_complete = driver->echo_bytes[0] == LIN_SYNC_BYTE &&
                driver->echo_bytes[1] == frame->pid;

            break;
        case LIN_MASTER_TX_RESPONSE:
            if (driver->echo_size < frame->length + 3) return;
            for (uint8_t i = 0; i < frame->length + 1; ++i) {
                if (driver->echo_bytes[2 + i] != frame->response_bytes[i]) {
                    is_complete = false;
                    break;
                }
            }
            break;
        case LIN_MASTER_RX_RESPONSE:
            if (driver->echo_size < frame->length + 3) return;
            uint8_t expected_checksum = 0;

            HAL_StatusTypeDef calc_status = LIN_calculate_checksum(
                frame->checksum_type,
                frame->pid,
                frame->length,
                driver->echo_bytes + 2,
                &expected_checksum
            );

            if (calc_status == HAL_OK) {
                if (expected_checksum == driver->echo_bytes[2 + frame->length]) {
                    frame->rx_valid_response = true;
                    memcpy(frame->response_bytes, driver->echo_bytes + 2, frame->length + 1);
                    
                }
                else {
                    frame->rx_valid_response = false;
                    is_complete = false;
                }
            }
            else {
                frame->rx_valid_response = false;
                is_complete = false;
            } 
            break;
        default:
            return;
    }

    LIN_master_state_machine(txn, is_complete);

    if (is_complete && txn->master_state == LIN_MASTER_TX_RESPONSE) {
        if (frame->tx_update_pending) {
            frame->tx_update_pending = false;
            memcpy(frame->response_bytes, frame->update_buffer, frame->length);
        }

        HAL_StatusTypeDef dma_status = LIN_calculate_checksum(
            frame->checksum_type,
            frame->pid,
            frame->length,
            frame->response_bytes,
            frame->response_bytes + frame->length);

        if (dma_status == HAL_OK) {
            HAL_UART_Transmit_DMA(
                driver->huart,
                frame->response_bytes,
                frame->length + 1);
            // TODO: Handle DMA error
        }
    }
}

static uint8_t get_dma_buffer_byte(LIN_Driver_t *driver, uint16_t index) {
    return driver->dma_buffer[index % LIN_DMA_BUFFER_SIZE];
}

static uint16_t count_dma_buffer_rx_bytes(LIN_Driver_t *driver, uint16_t write_index) {
    if (driver->dma_index <= write_index) {
        return write_index - driver->dma_index;
    }

    return LIN_DMA_BUFFER_SIZE - driver->dma_index + write_index;
}

static uint16_t get_dma_buffer_frame_start(LIN_Driver_t *driver, uint16_t write_index) {
    uint16_t index = driver->dma_index;
    uint16_t n_rx_bytes = count_dma_buffer_rx_bytes(driver, write_index);

    for (uint16_t n = 0; n < n_rx_bytes; ++n) {
        if (get_dma_buffer_byte(driver, index) == LIN_SYNC_BYTE) {
            return index % LIN_DMA_BUFFER_SIZE;
        }

        ++index;
    }

    return UINT16_MAX;
}

void LIN_HAL_TIM_PeriodElapsedCallback(
    LIN_Driver_t *driver,
    TIM_HandleTypeDef *htim) {
    if (htim == NULL || driver == NULL || htim != driver->htim) return;

    driver->schedule.elapsed_us += LIN_TIMEBASE_US;
    LIN_Slot_t *slot = &(driver->schedule.slots[driver->schedule.slot_index]);
    LIN_Transaction_t *txn = &(driver->transaction);

    if (driver->schedule.elapsed_us >= slot->duration_us) {
        uint16_t next_index = (driver->schedule.slot_index + 1) % driver->schedule.slot_count;

        if (txn->master_state == LIN_MASTER_SUCCESS) {
            memset(driver->echo_bytes, 0, LIN_MAX_DATA_SIZE + 3);
            driver->echo_size = 0;

            driver->schedule.slot_index = next_index;
            driver->schedule.elapsed_us = 0;

            txn->master_state = LIN_MASTER_TX_BREAK;

            LIN_Slot_t *next_slot = &(driver->schedule.slots[next_index]);
            LIN_Frame_t *next_frame = &(driver->schedule.frames[next_slot->frame_index]);
            txn->direction = next_frame->direction;
            HAL_LIN_SendBreak(driver->huart);
            // TODO: Handle DMA error
        }
        else if (txn->master_state == LIN_MASTER_ERROR) {
            // TODO: Error handling
       }
        else {
            // TODO: Timeout handling
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

    uint16_t n_rx_bytes = count_dma_buffer_rx_bytes(driver, Size);
    if (n_rx_bytes == 0) return;

    LIN_Transaction_t *txn = &(driver->transaction);
    if (txn->master_state == LIN_MASTER_IDLE) {
        // Discard junk bytes
        driver->dma_index = Size % LIN_DMA_BUFFER_SIZE;
        return;
    }

    // For reading a new frame
    if (driver->echo_size == 0) {
        uint16_t start_index = get_dma_buffer_frame_start(driver, Size);

        // No sync byte found in new bytes
        if (start_index == UINT16_MAX) {
            // Discard junk bytes
            driver->dma_index = Size % LIN_DMA_BUFFER_SIZE;
            return;
        }
        
        while (driver->dma_index != start_index) {
            driver->dma_index = (driver->dma_index + 1) % LIN_DMA_BUFFER_SIZE;
        }
        
        // Count new bytes from new start
        n_rx_bytes = count_dma_buffer_rx_bytes(driver, Size);
    }

    LIN_Slot_t *slot = &(driver->schedule.slots[driver->schedule.slot_index]);
    LIN_Frame_t *frame = &(driver->schedule.frames[slot->frame_index]);

    uint16_t n_expected_bytes = 0;
    
    if (txn->master_state == LIN_MASTER_TX_HEADER) {
        n_expected_bytes = 2;
    }
    else if (txn->master_state == LIN_MASTER_TX_RESPONSE ||
             txn->master_state == LIN_MASTER_RX_RESPONSE) {
        n_expected_bytes = frame->length + 1;
    }

    uint16_t n_bytes_left = (frame->length + 3) - driver->echo_size;

    uint16_t n_read_bytes = 0;

    if (n_expected_bytes < n_rx_bytes) {
        n_read_bytes = n_expected_bytes;
    }
    else if (n_bytes_left < n_rx_bytes) {
        n_read_bytes = n_bytes_left;
    }
    else {
        n_read_bytes = n_rx_bytes;
    }

    for (uint16_t i = 0; i < n_read_bytes; ++i) {
        driver->echo_bytes[i + driver->echo_size] = get_dma_buffer_byte(
            driver, driver->dma_index + i);
    }

    driver->echo_size += n_read_bytes;
    driver->dma_index = (driver->dma_index + n_read_bytes) % LIN_DMA_BUFFER_SIZE;

    LIN_read_dma_buffer(driver);
}

void LIN_HAL_UART_IRQHandler(
    LIN_Driver_t *driver,
    UART_HandleTypeDef *huart) {
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_LBDF) != RESET) {
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_LBDF);
        // UART detects break as a framing error
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_FE);

        LIN_Transaction_t *txn = &(driver->transaction);

        if (txn->master_state == LIN_MASTER_TX_BREAK) {
            LIN_master_state_machine(txn, true);

            LIN_Slot_t *slot = &(driver->schedule.slots[driver->schedule.slot_index]);
            LIN_Frame_t *frame = &(driver->schedule.frames[slot->frame_index]);

            frame->header_bytes[0] = LIN_SYNC_BYTE;
            frame->header_bytes[1] = frame->pid;

            HAL_UART_Transmit_DMA(huart, frame->header_bytes, 2);
            // TODO: Handle DMA error
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
    // Stop transmission and clear error flags
    HAL_UART_DMAStop(huart);
    // Reset DMA buffer
    memset(driver->dma_buffer, 0, LIN_DMA_BUFFER_SIZE);
    driver->dma_index = 0;
    // Rearm DMA for reception
    HAL_UARTEx_ReceiveToIdle_DMA(huart, driver->dma_buffer, LIN_DMA_BUFFER_SIZE);
    // TODO: Handle DMA error
}