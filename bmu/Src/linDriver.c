#include "stm32f7xx_hal.h"
#include "linDriver.h"
#include "string.h"
#include "stdint.h"
#include "math.h"

HAL_StatusTypeDef LIN_init_slot(
    LIN_ScheduleSlot_t *slot,
    uint8_t frameID,
    uint8_t data_length,
    LIN_Direction_t direction,
    LIN_ChecksumType_t cks_type,
    uint8_t *data) {
    if (slot == NULL) {
        return HAL_ERROR;
    }

    if (frameID > 0x3F || 
        cks_type == LIN_CHECKSUM_ENHANCED && (frameID == 0x3C || frameID == 0x3D)) {
        return HAL_ERROR;
    }

    if (data_length > LIN_MAX_DATA_SIZE || (data_length == 0 && data != NULL)) {
        return HAL_ERROR;
    } 

    slot->pID = LIN_calculate_PID(frameID);
    slot->data_length = data_length;
    slot->duration_ms = round(LIN_calculate_duration_us(slot)/LIN_TIMEBASE_MS)*LIN_TIMEBASE_MS;
    slot->direction = direction;
    slot->cks_type = cks_type;
    slot->data = data;

    return HAL_OK;
}

HAL_StatusTypeDef LIN_init_schedule(
    LIN_ScheduleManager_t *schedule_mgr,
    LIN_ScheduleSlot_t *table,
    uint8_t size) {
    if (schedule_mgr == NULL || table == NULL || size == 0) {
        return HAL_ERROR;
    }

    schedule_mgr->table = table;
    schedule_mgr->total_slots = size;
    schedule_mgr->current_slot_idx = 0;

    return HAL_OK;
}

HAL_StatusTypeDef LIN_wakeup_bus(
    GPIO_TypeDef *portType,
    uint16_t portNum,
    TIM_HandleTypeDef *htim) {
    HAL_GPIO_WritePin(portType, portNum, GPIO_PIN_RESET);
    delay_us(htim, LIN_TBIT_US*13);
    HAL_GPIO_WritePin(portType, portNum, GPIO_PIN_SET);

    return HAL_OK;
}

HAL_StatusTypeDef LIN_transmit_header(
    UART_HandleTypeDef *huart,
    LIN_ScheduleManager_t *schedule_mgr
) {
    if (huart == NULL || schedule_mgr == NULL) {
        return HAL_ERROR;
    }

    if (HAL_LIN_SendBreak(huart) != HAL_OK) {
        return HAL_ERROR;
    }

    LIN_ScheduleSlot_t slot = schedule_mgr->table[schedule_mgr->current_slot_idx];
    schedule_mgr->buffer[0] = LIN_SYNC_BYTE;
    schedule_mgr->buffer[1] = slot.pID;

    return HAL_UART_Transmit_DMA(huart, schedule_mgr->buffer, 2);
}

HAL_StatusTypeDef LIN_transmit_data(
    UART_HandleTypeDef *huart,
    LIN_ScheduleManager_t *schedule_mgr
) {
    if (huart == NULL || schedule_mgr == NULL) {
        return HAL_ERROR;
    }

    uint8_t current_slot_idx = schedule_mgr->current_slot_idx;
    LIN_ScheduleSlot_t *slot = &schedule_mgr->table[current_slot_idx];
    
    memcpy(schedule_mgr->buffer, slot->data, slot->data_length);

    schedule_mgr->buffer[slot->data_length] = LIN_calculate_checksum(
        slot->pID,
        slot->data,
        slot->data_length);

    return HAL_UART_Transmit_DMA(huart, schedule_mgr->buffer, slot->data_length + 1);
}

HAL_StatusTypeDef LIN_transmit_frame(
    UART_HandleTypeDef *huart,
    LIN_ScheduleManager_t *schedule_mgr) {
    HAL_StatusTypeDef htx_status = LIN_transmit_header(huart, schedule_mgr);

    if (htx_status != HAL_OK) {
        return htx_status;
    }

    HAL_StatusTypeDef dtx_status = LIN_transmit_data(huart, schedule_mgr);

    __HAL_UART_SEND_REQ(huart, UART_RXDATA_FLUSH_REQUEST);

    return dtx_status;
}

HAL_StatusTypeDef LIN_receive_frame(
    UART_HandleTypeDef *huart,
    LIN_ScheduleManager_t *schedule_mgr) {
    HAL_StatusTypeDef htx_status = LIN_transmit_header(huart, schedule_mgr);

    if (htx_status != HAL_OK) {
        return htx_status;
    }

    uint8_t current_slot_idx = schedule_mgr->current_slot_idx;
    LIN_ScheduleSlot_t *slot = &schedule_mgr->table[current_slot_idx];

    HAL_StatusTypeDef drx_status = HAL_UART_Receive_DMA(huart, schedule_mgr->buffer, slot->data_length + 3);

    uint8_t checksum = schedule_mgr->buffer[slot->data_length + 2];
    uint8_t *data;

    memcpy(data, schedule_mgr->buffer + 2, slot->data_length);

    return LIN_validate_checksum(slot->pID, data, slot->data_length, checksum);
}

HAL_StatusTypeDef LIN_validate_checksum(
    uint8_t pID,
    uint8_t *data,
    uint8_t data_length,
    uint8_t checksum) {
    if (LIN_calculate_checksum(pID, data, data_length) == checksum) {
        return HAL_OK;
    }

    return HAL_ERROR;
}

HAL_StatusTypeDef LIN_execute_schedule(UART_HandleTypeDef *huart, LIN_ScheduleManager_t *schedule_mgr) {
    if (huart == NULL || schedule_mgr == NULL || schedule_mgr->table == NULL) {
        return HAL_ERROR;
    }

    uint8_t current_slot_idx = schedule_mgr->current_slot_idx;
    LIN_ScheduleSlot_t *slot = &schedule_mgr->table[current_slot_idx];

    if (slot->direction == LIN_MASTER_SUBSCRIBE) {
        HAL_StatusTypeDef status = LIN_receive_frame(huart, schedule_mgr);

        if (status == HAL_OK && slot->data != NULL) {
            memcpy(slot->data, schedule_mgr->buffer + 2, slot->data_length);
        }

        return status;
    }
    
    return LIN_transmit_frame(huart, schedule_mgr);
}

uint8_t LIN_calculate_checksum(uint8_t pID, uint8_t *data, uint8_t data_length) {
    uint16_t sum = 0;

    if (pID != 0x3C && pID != 0x3D) {
        sum = pID;
    }
    
    for (uint8_t i = 0; i < data_length; ++i) {
        sum += data[i];
        if (sum > 0xFF) {
            sum = sum - 0xFF;
        }
    }

    return (uint8_t)(~sum);
}

uint8_t LIN_calculate_PID(uint8_t frameID) {
    uint8_t p0 = (frameID ^ (frameID >> 1) ^ (frameID >> 2) ^ (frameID >> 4)) & 1U;
    uint8_t p1 = ~(((frameID >> 1) ^ (frameID >> 3) ^ (frameID >> 4) ^ (frameID >> 5)) & 1U);

    return (p1 << 7) | (p0 << 6) | frameID;
}

uint16_t LIN_calculate_duration_ms(LIN_ScheduleSlot_t *slot) {
    // Nominal transmission times * 1.4
    uint16_t t_header = 47.6*LIN_TBIT_US;
    uint16_t t_response = 14*(slot->data_length + 1)*LIN_TBIT_US;
    return (t_header + t_response) / 1000;
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