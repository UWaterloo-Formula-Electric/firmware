#include "stm32f7xx_hal.h"
#include "linDriver.h"
#include "string.h"
#include "stdint.h"

HAL_StatusTypeDef LIN_init_slot(
    LIN_ScheduleSlot_t *slot,
    uint8_t frameID,
    uint8_t data_length,
    uint16_t duration_ms,
    LIN_Direction_t direction,
    LIN_ChecksumType_t cks_type,
    uint8_t *p_data_buffer
) {
    if (slot == NULL) return HAL_ERROR;
    if (cks_type == LIN_CHECKSUM_ENHANCED && (frameID == 0x3C || frameID == 0x3D)) return HAL_ERROR;
    if (duration_ms / LIN_TIMEBASE_MS < 1) return HAL_ERROR;
    if (data_length > LIN_MAX_DATA_SIZE || (data_length == 0 && p_data_buffer != NULL)) return HAL_ERROR;

    slot->pid = LIN_calculate_PID(frameID);
    slot->data_length = data_length;
    slot->duration_ms = duration_ms;
    slot->direction = direction;
    slot->cks_type = cks_type;
    slot->p_data_buffer = p_data_buffer;

    return HAL_OK;
}

HAL_StatusTypeDef LIN_init_schedule(
    LIN_ScheduleManager_t *schedule_mgr,
    LIN_ScheduleSlot_t *table,
    uint8_t size
) {
    if (schedule_mgr == NULL || table == NULL || size == 0) return HAL_ERROR;

    schedule_mgr->table = table;
    schedule_mgr->total_slots = size;
    schedule_mgr->current_slot_idx = 0;

    return HAL_OK;
}

HAL_StatusTypeDef LIN_init_frame(LIN_Frame_t *frame, LIN_ScheduleSlot_t *slot) {
    if (frame == NULL || slot == NULL) return HAL_ERROR;

    frame->pid = slot->pid;
    frame->data_length = slot->data_length;
    
    if (slot->p_data_buffer != NULL && slot->direction == LIN_MASTER_PUBLISH) {
        memcpy(frame->data, slot->p_data_buffer, slot->data_length);
        frame->checksum = LIN_calculate_checksum(frame);
    }

    return HAL_OK;
}

HAL_StatusTypeDef LIN_wakeup_bus(GPIO_TypeDef *portType, uint16_t portNum, TIM_HandleTypeDef *htim) {
    HAL_GPIO_WritePin(portType, portNum, GPIO_PIN_RESET);
    delay_us(htim, TLIN1029Q1_LIN_BUS_WAKEUP_US);

    return HAL_OK;
}

HAL_StatusTypeDef LIN_transmit_frame(UART_HandleTypeDef *huart, const LIN_Frame_t *frame) {
    if (huart == NULL || frame == NULL || frame->data_length > LIN_MAX_DATA_SIZE) {
        return HAL_ERROR;
    }

    if (HAL_LIN_SendBreak(huart) != HAL_OK) {
        return HAL_ERROR;
    }

    uint8_t tx_size = frame->data_length + 3;
    uint8_t buffer[LIN_MAX_DATA_SIZE + 3];

    buffer[0] = LIN_SYNC_BYTE;
    buffer[1] = frame->pid;

    memcpy(&buffer[2], frame->data, frame->data_length);

    buffer[frame->data_length + 2] = frame->checksum;

    HAL_StatusTypeDef status = HAL_UART_Transmit(huart, buffer, tx_size, 100);

    __HAL_UART_SEND_REQ(huart, UART_RXDATA_FLUSH_REQUEST);

    return status;
}

HAL_StatusTypeDef LIN_receive_frame(UART_HandleTypeDef *huart, LIN_Frame_t *frame) {
    if (huart == NULL || frame == NULL || frame->data_length > LIN_MAX_DATA_SIZE) {
        return HAL_ERROR;
    }

    if (HAL_LIN_SendBreak(huart) != HAL_OK) {
        return HAL_ERROR;
    }

    uint8_t tx_buffer[2] = { LIN_SYNC_BYTE, frame->pid };
    if (HAL_UART_Transmit(huart, tx_buffer, 2, HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }

    uint8_t rx_size = frame->data_length + 2;
    uint8_t rx_buffer[LIN_MAX_DATA_SIZE + 2];

    if (HAL_UART_Receive(huart, rx_buffer, rx_size, HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }

    memcpy(frame->data, rx_buffer + 2, frame->data_length);
    frame->checksum = rx_buffer[frame->data_length + 1];

    return LIN_validate_checksum(frame);
}

HAL_StatusTypeDef LIN_validate_checksum(const LIN_Frame_t *frame) {
    if (frame == NULL) {
        return HAL_ERROR;
    }

    if (LIN_calculate_checksum((LIN_Frame_t*)frame) == frame->checksum) {
        return HAL_OK;
    }

    return HAL_ERROR;
}

HAL_StatusTypeDef LIN_execute_schedule(UART_HandleTypeDef *huart, LIN_ScheduleManager_t *schedule_mgr) {
    if (huart == NULL || schedule_mgr == NULL || schedule_mgr->table == NULL) {
        return HAL_ERROR;
    }

    uint8_t current_slot_idx = schedule_mgr->current_slot_idx;
    LIN_ScheduleSlot_t *p_slot = &schedule_mgr->table[current_slot_idx];

    LIN_Frame_t frame;
    LIN_init_frame(&frame, p_slot);

    if (p_slot->direction == LIN_MASTER_SUBSCRIBE) {
        HAL_StatusTypeDef status = LIN_receive_frame(huart, &frame);

        if (status == HAL_OK && p_slot->p_data_buffer != NULL) {
            memcpy(p_slot->p_data_buffer, frame.data, frame.data_length);
        }

        return status;
    }
    
    return LIN_transmit_frame(huart, &frame);
}

uint8_t LIN_calculate_checksum(const LIN_Frame_t *frame) {
    uint16_t sum = 0;

    if (frame->pid != 0x3C && frame->pid != 0x3D) {
        sum = frame->pid;
    }
    
    for (uint8_t i = 0; i < frame->data_length; ++i) {
        sum += frame->data[i];
        if (sum > 0xFF) {
            sum = sum - 0xFF;
        }
    }

    sum = 0xFF - sum;

    return (uint8_t)(~sum);
}

uint8_t LIN_calculate_PID(uint8_t frameID) {
    uint8_t p0 = (frameID ^ (frameID >> 1) ^ (frameID >> 2) ^ (frameID >> 4)) & 1U;
    uint8_t p1 = ~(((frameID >> 1) ^ (frameID >> 3) ^ (frameID >> 4) ^ (frameID >> 5)) & 1U);

    return (p1 << 7) | (p0 << 6) | frameID;
}

void delay_us(TIM_HandleTypeDef *htim, uint8_t duration) {
    uint16_t start_time_us = __HAL_TIM_GET_COUNTER(htim);

    while (__HAL_TIM_GET_COUNTER(htim) - start_time_us < duration) {

    }
}

void LIN_HAL_TIM_Callback(LIN_ScheduleManager_t *schedule_mgr) {
    if (schedule_mgr == NULL || schedule_mgr->total_slots == 0){
        return;
    }

    static uint16_t time_executing_ms = 0;
    time_executing_ms += LIN_TIMEBASE_MS;

    uint8_t current_slot_idx = schedule_mgr->current_slot_idx;
    uint16_t current_slot_duration_ms = schedule_mgr->table[current_slot_idx].duration_ms;

    if (time_executing_ms >= current_slot_duration_ms) {
        time_executing_ms = 0;
        schedule_mgr->current_slot_idx = (schedule_mgr->current_slot_idx + 1) % schedule_mgr->total_slots;
    }
}