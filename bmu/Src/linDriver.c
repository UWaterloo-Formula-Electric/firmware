#include "stm32f7xx_hal.h"
#include "linDriver.h"
#include "string.h"
#include "stdint.h"
#include "stdbool.h"

LIN_HandleTypeDef hlin = {
    .schedule_manager = {
        .table = NULL,
        .n_slots = 0,
        .schedule_slot_idx = 0,
        .active_slot_idx = 0
    },
    .rx_buffer_idx = 0,
    .frame_state = LIN_IDLE,
    .frame_result = LIN_PENDING
};

HAL_StatusTypeDef LIN_init_frame(
    LIN_Frame_t *frame,
    uint8_t frameID,
    uint8_t *data,
    uint8_t len_data) {
    if (frame == NULL || frameID > 0x3E ||
        len_data > LIN_MAX_DATA_SIZE || len_data == 0 ||
        (len_data > 0 && data == NULL)) {
        return HAL_ERROR;
    }

    frame->pID = LIN_calculate_PID(frameID);
    frame->len_data = len_data;
    frame->data = data;
    frame->checksum = LIN_calculate_checksum(
        frame->pID,
        frame->data,
        frame->len_data);

    return HAL_OK;
 }

HAL_StatusTypeDef LIN_init_slot(
    LIN_ScheduleSlot_t *slot,
    LIN_Direction_t direction,
    LIN_Frame_t *frame
) {
    if (slot == NULL || frame == NULL) {
        return HAL_ERROR;
    }

    slot->duration_ms = LIN_calculate_slot_duration_ms(frame->len_data);
    slot->n_tries = 0;
    slot->direction = direction;
    slot->frame = frame;

    return HAL_OK;
}

HAL_StatusTypeDef LIN_init_schedule(
    LIN_ScheduleManager_t *schedule_manager,
    LIN_ScheduleSlot_t *table,
    uint8_t n_slots) {
    if (schedule_manager == NULL || table == NULL || n_slots == 0) {
        return HAL_ERROR;
    }

    schedule_manager->table = table;
    schedule_manager->n_slots = n_slots;
    schedule_manager->schedule_slot_idx = 0;
    schedule_manager->active_slot_idx = 0;

    return HAL_OK;
}

LIN_ScheduleSlot_t* get_active_slot() {
    if (hlin.schedule_manager.table == NULL || 
        hlin.schedule_manager.active_slot_idx >= hlin.schedule_manager.n_slots) {
        return NULL;
    }

    uint8_t idx = hlin.schedule_manager.active_slot_idx;
    return &hlin.schedule_manager.table[idx];
}

uint8_t get_rx_buffer_byte(uint16_t start, uint16_t offset) {
    return hlin.rx_buffer[(start + offset) % LIN_RX_BUFFER_SIZE];
}

HAL_StatusTypeDef LIN_init_driver(TIM_HandleTypeDef *htim, UART_HandleTypeDef *huart) {
    if (htim == NULL || huart == NULL) {
        return HAL_ERROR;
    }

    hlin.config.huart = huart;
    hlin.config.htim = htim;
    hlin.rx_buffer_idx = 0;
    hlin.frame_state = LIN_IDLE;
    hlin.frame_result = LIN_PENDING;

    // Enable Line Break Detection (LBD) interrupt
    __HAL_UART_ENABLE_IT(huart, UART_IT_LBD);

    // Start background reception via DMA
    return HAL_UARTEx_ReceiveToIdle_DMA(huart, hlin.rx_buffer, LIN_RX_BUFFER_SIZE);
}

HAL_StatusTypeDef LIN_transmit_break( UART_HandleTypeDef *huart) {
    if (huart == NULL) {
        return HAL_ERROR;
    }
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_TCF);
    
    // Signal start of new frame
    return HAL_LIN_SendBreak(huart);
}

HAL_StatusTypeDef LIN_transmit_response(UART_HandleTypeDef *huart, LIN_Frame_t *frame) {
    if (huart == NULL || frame == NULL) {
        return HAL_ERROR;
    }

    memcpy(hlin.tx_buffer, frame->data, frame->len_data);
    hlin.tx_buffer[frame->len_data] = frame->checksum;

    return HAL_UART_Transmit_DMA(huart, hlin.tx_buffer, frame->len_data + 1);
}

uint8_t LIN_calculate_checksum(
    const uint8_t pID,
    const uint8_t *response_bytes,
    const uint8_t response_length) {
    uint16_t sum = 0;
    uint8_t frame_id = pID & 0x3FU;

    // Frame IDs 60 and 61 use classic checksum
    if (frame_id != 0x3CU && frame_id != 0x3DU) {
        sum = pID;
    }
    // Inverted 8-bit sum with carry
    for (uint8_t i = 0; i < response_length; ++i) {
        sum += response_bytes[i];
        if (sum > 0xFF) {
            sum -= 0xFF;
        }
    }

    return (uint8_t)(~sum);
}

uint8_t LIN_calculate_PID(const uint8_t frameID) {
    uint8_t p0 = (frameID ^ (frameID >> 1) ^ (frameID >> 2) ^ (frameID >> 4)) & 1U;
    uint8_t p1 = (~((frameID >> 1) ^ (frameID >> 3) ^ (frameID >> 4) ^ (frameID >> 5))) & 1U;

    return (p1 << 7) | (p0 << 6) | frameID;
}

uint32_t LIN_calculate_header_transmission_time_us(void) {
    return 34 * LIN_TBIT_US;
}

uint32_t LIN_calculate_response_transmission_time_us(const uint8_t response_length) {
    return (response_length + 1) * 10 * LIN_TBIT_US;
}

uint32_t LIN_calculate_slot_duration_ms(const uint8_t response_length) {
    uint32_t t_header_us = LIN_calculate_header_transmission_time_us();
    uint32_t t_response_us = LIN_calculate_response_transmission_time_us(response_length);

    uint64_t total_us = ((uint64_t)t_header_us + t_response_us) * LIN_BUFFER_FACTOR;
    total_us = (total_us + 9) / 10;

    const uint32_t timebase_us = LIN_TIMEBASE_MS * 1000;

    return (uint32_t)((total_us + timebase_us - 1) / timebase_us) * LIN_TIMEBASE_MS;
}

void LIN_advance_schedule() {
    LIN_ScheduleManager_t *schedule_manager = &hlin.schedule_manager;

    schedule_manager->schedule_slot_idx = ((schedule_manager->active_slot_idx + 1)
                                                    % schedule_manager->n_slots);
    schedule_manager->active_slot_idx = schedule_manager->schedule_slot_idx;

    hlin.frame_state = LIN_TX_BREAK;
    hlin.frame_result = LIN_PENDING;

    LIN_transmit_break(hlin.config.huart);
}

void LIN_HAL_TIM_Callback(void) {
    static uint32_t t_start_us = 0;
    static uint32_t elapsed_ms = 0;

    uint32_t t_end_us = __HAL_TIM_GET_COUNTER(hlin.config.htim);

    elapsed_ms += (t_end_us - t_start_us) / 1000;
    t_start_us = t_end_us;

    LIN_ScheduleSlot_t *slot = get_active_slot();
    uint32_t allocated_ms = slot->duration_ms;

    if (elapsed_ms < allocated_ms &&
        hlin.frame_state == LIN_RECOVERY) {
        if (slot->n_tries < LIN_MAX_TRIES) {
            ++slot->n_tries;

            elapsed_ms = 0;
            hlin.frame_result = LIN_PENDING;

            hlin.frame_state = LIN_TX_BREAK;
            LIN_transmit_break(hlin.config.huart);
        }
        else {
            LIN_advance_schedule();
        }
        return;
    }

    if (elapsed_ms >= allocated_ms) {
        LIN_advance_schedule();
    }
}

void LIN_HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart != hlin.config.huart) {
        return;
    }

    LIN_ScheduleSlot_t *slot = get_active_slot();
    LIN_Frame_t *frame = slot->frame;

    if (hlin.frame_state == LIN_TX_BREAK) {
        uint16_t dma_remaining = __HAL_DMA_GET_COUNTER(huart->hdmarx);
        hlin.rx_buffer_idx = LIN_RX_BUFFER_SIZE - dma_remaining;

        hlin.tx_buffer[0] = LIN_SYNC_BYTE;
        hlin.tx_buffer[1] = frame->pID;
        
        hlin.frame_state = LIN_TX_SYNC_PID;
        HAL_UART_Transmit_DMA(huart, hlin.tx_buffer, 2);
    }
    else if (hlin.frame_state == LIN_TX_SYNC_PID) {
        if (slot->direction == MASTER_PUBLISH) {
            hlin.frame_state = LIN_TX_RESPONSE;
            LIN_transmit_response(huart, slot->frame);
        }
        else {
            hlin.frame_state = LIN_RX_RESPONSE;
        }
    }
}

void LIN_HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
    if (huart != hlin.config.huart) {
        return;
    }
    // Skip if physical and logical bus are idle
    if (hlin.frame_state == LIN_IDLE ||
        hlin.frame_state == LIN_READY ||
        hlin.frame_state == LIN_RECOVERY) {
        return;
    }

    LIN_ScheduleSlot_t *slot = get_active_slot();
    LIN_Frame_t *frame = slot->frame;

    uint16_t start = hlin.rx_buffer_idx;
    uint16_t n_rx_bytes = 0;

    if (size >= start) {
        n_rx_bytes = size - start;
    }
    else {
        n_rx_bytes = LIN_RX_BUFFER_SIZE - start + size;
    }

    uint16_t n_expected_bytes = 3 + frame->len_data;
    // Wait to receive complete frame
    if (n_rx_bytes < n_expected_bytes) {
        return; 
    }

    bool is_valid_frame = true;
    // Verify sync and PID echo
    if (get_rx_buffer_byte(start, 0) != LIN_SYNC_BYTE ||
        get_rx_buffer_byte(start, 1) != frame->pID) {
        hlin.frame_result = LIN_BIT_ERROR;
        is_valid_frame = false;
    }

    if (slot->direction == MASTER_PUBLISH) {
        // Verify data echo
        for (uint8_t i = 0; i < frame->len_data; i++) {
            if (get_rx_buffer_byte(start, 2 + i) != frame->data[i]) {
                hlin.frame_result = LIN_BIT_ERROR;
                is_valid_frame = false;
                break;
            }
        }
        // Verify checksum echo
        if (get_rx_buffer_byte(start, 2 + frame->len_data) != frame->checksum) {
            hlin.frame_result = LIN_BIT_ERROR;
            is_valid_frame = false;
        }
    } 
    else if (slot->direction == MASTER_SUBSCRIBE) {
        uint8_t received_checksum = get_rx_buffer_byte(start, 2 + frame->len_data);
        uint8_t calculated_checksum = LIN_calculate_checksum(
            frame->pID,
            frame->data,
            frame->len_data);
        
        // Update slot frame with valid data
        if (received_checksum == calculated_checksum) {
            for (uint8_t i = 0; i < frame->len_data; i++) {
                frame->data[i] = get_rx_buffer_byte(start, 2 + i);
            }

            frame->checksum = received_checksum;
        } else {
            hlin.frame_result = LIN_CHECKSUM_ERROR;
        }
    }

    if (is_valid_frame) {
        hlin.frame_result = LIN_SUCCESS;
        hlin.frame_state = LIN_READY;
    }
    else {
        hlin.frame_state = LIN_RECOVERY;
    }
    // Start new frame after current frame
    hlin.rx_buffer_idx = size % LIN_RX_BUFFER_SIZE; 
}

void LIN_HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart != hlin.config.huart) return;

    uint32_t error_code = HAL_UART_GetError(huart);

    if ((error_code & HAL_UART_ERROR_FE) && (hlin.frame_state == LIN_TX_BREAK)) {
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF);
        return; 
    }
    // Reflect error in frame result
    if (error_code & HAL_UART_ERROR_ORE) {
        hlin.frame_result = LIN_OVERRUN_ERROR;
    } else if (error_code & (HAL_UART_ERROR_FE | HAL_UART_ERROR_NE)) {
        hlin.frame_result = LIN_BIT_ERROR;
    } else if (error_code & HAL_UART_ERROR_DMA) {
        hlin.frame_result = LIN_DMA_ERROR;
    }
    // Clear error flags for next tx
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_PEF);
    HAL_UART_AbortReceive(huart);
    // Recover in next timer callback
    hlin.frame_state = LIN_RECOVERY; 
    // Listen to rx for next frame
    HAL_UARTEx_ReceiveToIdle_DMA(huart, hlin.rx_buffer, LIN_RX_BUFFER_SIZE);
}