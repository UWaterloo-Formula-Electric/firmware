#include "stm32f7xx_hal.h"
#include "linDriver.h"
#include "string.h"
#include "stdint.h"
#include "stdbool.h"

LIN_HandleTypeDef hlin = {
    .config = {
        .htim = NULL,
        .huart = NULL,
    },
    .schedule_manager = {
        .table = NULL,
        .n_slots = 0,
        .schedule_slot_idx = 0,
        .active_slot_idx = 0,
        .elapsed_ms = 0
    },
    .frame_state = LIN_IDLE,
    .frame_result = LIN_PENDING
};

HAL_StatusTypeDef LIN_init_frame(
    LIN_Frame_t *frame,
    uint8_t frameID,
    uint8_t *response_bytes,
    uint8_t response_length) {
    if (frame == NULL || frameID > 0x3E ||
        response_length > LIN_MAX_DATA_SIZE || response_length == 0) {
        return HAL_ERROR;
    }

    frame->header_bytes[0] = LIN_SYNC_BYTE;

    uint8_t pID = LIN_calculate_PID(frameID);

    frame->header_bytes[1] = pID;
    
    if (response_bytes != NULL) {
        memcpy(frame->response_bytes, response_bytes, response_length);

        frame->response_bytes[response_length] = LIN_calculate_checksum(
            pID,
            response_bytes,
            response_length
        );
    }

    return HAL_OK;
 }

HAL_StatusTypeDef LIN_init_slot(
    LIN_ScheduleSlot_t *slot,
    LIN_Direction_t direction,
    LIN_FrameType_t frame_type,
    LIN_Frame_t *frame
) {
    if (slot == NULL || frame == NULL) {
        return HAL_ERROR;
    }

    slot->duration_ms = LIN_calculate_slot_duration_ms(frame->response_length);
    slot->direction = direction;
    slot->frame_type = frame_type;
    slot->frame = frame;

    return HAL_OK;
}


HAL_StatusTypeDef LIN_init_schedule(
    LIN_ScheduleManager_t *schedule_manager,
    LIN_ScheduleSlot_t *table,
    uint8_t n_slots) {
    if (table == NULL || n_slots == 0) {
        return HAL_ERROR;
    }

    schedule_manager->table = table;
    schedule_manager->n_slots = n_slots;
    schedule_manager->schedule_slot_idx = 0;
    schedule_manager->active_slot_idx = 0;
    schedule_manager->elapsed_ms = 0;

    return HAL_OK;
}

LIN_ScheduleSlot_t* get_active_slot() {
    uint8_t idx = hlin.schedule_manager.active_slot_idx;
    return &hlin.schedule_manager.table[idx];
}

HAL_StatusTypeDef LIN_wakeup(UART_HandleTypeDef *huart) {
    // Slave nodes detect break after 11 bit times
    return HAL_LIN_Init(huart, UART_LINBREAKDETECTLENGTH_11B);
}

HAL_StatusTypeDef LIN_transmit_header(TIM_HandleTypeDef *htim, UART_HandleTypeDef *huart, const LIN_Frame_t *frame) {
    if (huart == NULL || frame == NULL) {
        return HAL_ERROR;
    }

    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_TCF);
    // Signal start of new frame
    HAL_StatusTypeDef break_status = HAL_LIN_SendBreak(huart);

    if (break_status != HAL_OK) {
        return break_status;
    }

    // Wait to transmit 14 bits
    uint32_t delay_us = LIN_TBIT_US * LIN_BUFFER_FACTOR * 14;

    __HAL_TIM_SET_COUNTER(htim, 0);

    while (__HAL_TIM_GET_COUNTER(htim) < delay_us) {
        if (huart->Instance->ISR & USART_ISR_TC) {
            return HAL_UART_Transmit_DMA(huart, frame->header_bytes, 2);
        }
    }

    return HAL_TIMEOUT;
}

HAL_StatusTypeDef LIN_transmit_response(UART_HandleTypeDef *huart, const LIN_Frame_t *frame) {
    if (huart == NULL || frame == NULL) {
        return HAL_ERROR;
    }

    return HAL_UART_Transmit_DMA(huart, frame->response_bytes, frame->response_length);
}

uint8_t LIN_calculate_checksum(
    const uint8_t pID,
    const uint8_t *response_bytes,
    const uint8_t response_length) {
    uint16_t sum = 0;
    uint8_t frame_id = pID & 0x3FU;

    if (frame_id != 0x3CU && frame_id != 0x3DU) {
        sum = pID;
    }
    
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

void LIN_start_schedule(TIM_HandleTypeDef *htim, UART_HandleTypeDef *huart) {
    uint8_t slot_idx = hlin.schedule_manager.active_slot_idx;
    LIN_ScheduleSlot_t *slot = &hlin.schedule_manager.table[slot_idx];
    LIN_Frame_t *frame = slot->frame;

    HAL_StatusTypeDef tx_header_state = LIN_transmit_header(htim, huart, frame);

    if (tx_header_state == HAL_OK) {
        hlin.frame_state = LIN_TX_HEADER;
    }
}

void LIN_HAL_TIM_Callback(void) {
    LIN_ScheduleManager_t *schedule_manager = &hlin.schedule_manager;
    LIN_ScheduleSlot_t *slot = get_active_slot();

    schedule_manager->elapsed_ms += LIN_TIMEBASE_MS;

    uint32_t allocated_ms = slot->duration_ms;

    if (schedule_manager->elapsed_ms >= allocated_ms) {
        schedule_manager->schedule_slot_idx = (schedule_manager->active_slot_idx + 1) % schedule_manager->n_slots;

        if (hlin.frame_state != LIN_IDLE &&
            hlin.frame_state != LIN_READY &&
            hlin.frame_state != LIN_RECOVERY &&
            hlin.frame_result == LIN_PENDING) {
            hlin.frame_result = LIN_TIMEOUT;
        }

        if (hlin.frame_state == LIN_RECOVERY) {
            // TODO: Match logical and physical frame state
        }
        else if (hlin.frame_result != LIN_SUCCESS) {
            // TODO: Log transmission error
        }

        schedule_manager->active_slot_idx = schedule_manager->schedule_slot_idx;
        schedule_manager->elapsed_ms = 0;
        hlin.frame_result = LIN_PENDING;

        LIN_start_schedule(hlin.config.htim, hlin.config.huart);
    }
}

void LIN_HAL_UART_Tx_Callback(UART_HandleTypeDef *huart) {
    if (huart != hlin.config.huart) {
        return;
    }

    if (huart->ErrorCode != HAL_UART_ERROR_NONE) {
        hlin.frame_result = LIN_ERROR;
        return;
    }

    if (hlin.frame_state == LIN_TX_HEADER) {
        LIN_ScheduleSlot_t *slot = get_active_slot();
        LIN_Frame_t *frame= slot->frame;

        if (slot->direction == MASTER_SUBSCRIBE) {
            // Assume no echoed bytes in response
            HAL_StatusTypeDef rx_response_status = HAL_UART_Receive_DMA(
                huart,
                frame->response_bytes,
                frame->response_length + 1);

            if (rx_response_status == HAL_OK) {
                hlin.frame_state = LIN_RX_RESPONSE;
            }
            else {
                hlin.frame_result = LIN_ERROR;
            }
        } else {
            HAL_StatusTypeDef tx_response_status = LIN_transmit_response(huart, frame);

            if (tx_response_status == HAL_OK) {
                hlin.frame_state = LIN_TX_RESPONSE;
            }
            else {
                hlin.frame_result = LIN_ERROR;
            }
        }
    } else if (hlin.frame_state == LIN_TX_RESPONSE) {
        hlin.frame_state = LIN_READY;
        hlin.frame_result = LIN_SUCCESS;
    }
}

void LIN_HAL_UART_Rx_Callback(UART_HandleTypeDef *huart) {
    if (huart != hlin.config.huart) {
        return;
    }

    if (huart->ErrorCode != HAL_UART_ERROR_NONE) {
        hlin.frame_result = LIN_ERROR;
        return;
    }

    LIN_ScheduleSlot_t *slot = get_active_slot();
    LIN_Frame_t *frame = slot->frame;

    uint8_t checksum = frame->response_bytes[frame->response_length]; 

    bool is_valid_data = (LIN_calculate_checksum(
        frame->header_bytes[1],
        frame->response_bytes,
        frame->response_length) == checksum);
        
    if (is_valid_data) {
        hlin.frame_result = LIN_SUCCESS;
    } else {
        hlin.frame_result = LIN_ERROR;
    }

    hlin.frame_state = LIN_READY;
}