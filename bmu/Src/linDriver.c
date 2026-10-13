#include "stm32f7xx_hal.h"
#include "linDriver.h"
#include "string.h"
#include "stdint.h"
#include "stdbool.h"

LIN_ScheduleManager_t schedule_mgr = {
    .huart = NULL,
    .table = NULL,
    .total_slots = 0,
    .schedule_slot_idx = 0,
    .active_slot_idx = 0,
    .elapsed_ms = 0,
    .started_schedule = false,
    .lin_state = LIN_IDLE,
    .lin_result = LIN_PENDING
};

HAL_StatusTypeDef LIN_init_slot(
    LIN_ScheduleSlot_t *slot,
    uint8_t frameID,
    uint8_t data_length,
    LIN_Direction_t direction,
    uint8_t *data) {
    
    if (slot == NULL ||
        frameID > 0x3FU ||
        data_length == 0U ||
        data_length > LIN_MAX_DATA_SIZE ||
        data == NULL ||
        (direction != MASTER_PUBLISH && direction != MASTER_SUBSCRIBE)) {
        return HAL_ERROR;
    }

    slot->n_retries = 0;
    slot->direction = direction;
    slot->data_length = data_length;
    slot->data = data;
    slot->pID = LIN_calculate_PID(frameID);
    
    slot->duration_ms = LIN_calculate_slot_duration_ms(slot);
    if (slot->duration_ms == 0U) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef LIN_init_schedule(UART_HandleTypeDef *huart, LIN_ScheduleSlot_t *table, uint8_t total_slots) {
    if (huart == NULL || table == NULL || total_slots == 0U) {
        return HAL_ERROR;
    }

    for (uint8_t i = 0; i < total_slots; i++) {
        if (table[i].data_length == 0U || 
            table[i].data_length > LIN_MAX_DATA_SIZE || 
            table[i].data == NULL ||
            (table[i].direction != MASTER_PUBLISH && table[i].direction != MASTER_SUBSCRIBE) ||
            table[i].duration_ms == 0U) {
            return HAL_ERROR;
        }
    }

    schedule_mgr.huart = huart;
    schedule_mgr.table = table;
    schedule_mgr.total_slots = total_slots;
    schedule_mgr.started_schedule = false;
    schedule_mgr.schedule_slot_idx = 0U;
    schedule_mgr.active_slot_idx = 0U;
    schedule_mgr.elapsed_ms = 0U;
    schedule_mgr.lin_state = LIN_IDLE;
    schedule_mgr.lin_result = LIN_PENDING;

    return HAL_OK;
}

HAL_StatusTypeDef LIN_wakeup_bus(
    GPIO_TypeDef *portType,
    uint16_t portNum,
    TIM_HandleTypeDef *htim) {
    if (portType == NULL || htim == NULL) {
        return HAL_ERROR;
    }

    HAL_GPIO_WritePin(portType, portNum, GPIO_PIN_RESET);
    delay_us(htim, LIN_TBIT_US * 13);
    HAL_GPIO_WritePin(portType, portNum, GPIO_PIN_SET);

    return HAL_OK;
}

HAL_StatusTypeDef LIN_transmit_header(UART_HandleTypeDef *huart) {
    if (huart == NULL || schedule_mgr.table == NULL) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef break_status = HAL_LIN_SendBreak(huart);

    if (break_status != HAL_OK) {
        return break_status;
    }

    LIN_ScheduleSlot_t *slot = &schedule_mgr.table[schedule_mgr.active_slot_idx];
    schedule_mgr.buffer[0] = LIN_SYNC_BYTE;
    schedule_mgr.buffer[1] = slot->pID;

    return HAL_UART_Transmit_DMA(huart, schedule_mgr.buffer, 2);
}

HAL_StatusTypeDef LIN_transmit_data(UART_HandleTypeDef *huart) {
    if (huart == NULL || schedule_mgr.table == NULL) {
        return HAL_ERROR;
    }

    uint8_t current_slot_idx = schedule_mgr.active_slot_idx;
    LIN_ScheduleSlot_t *slot = &schedule_mgr.table[current_slot_idx];
    
    memcpy(schedule_mgr.buffer, slot->data, slot->data_length);

    schedule_mgr.buffer[slot->data_length] = LIN_calculate_checksum(
        slot->pID,
        slot->data,
        slot->data_length);

    return HAL_UART_Transmit_DMA(huart, schedule_mgr.buffer, slot->data_length + 1);
}

HAL_StatusTypeDef LIN_execute_slot(UART_HandleTypeDef *huart) {
    if (huart == NULL ||
        schedule_mgr.table == NULL ||
        schedule_mgr.total_slots == 0U ||
        schedule_mgr.active_slot_idx >= schedule_mgr.total_slots) {
        return HAL_ERROR;
    }

    if (schedule_mgr.lin_state != LIN_IDLE) {
        return HAL_BUSY;
    }

    HAL_StatusTypeDef status = LIN_transmit_header(huart);

    if (status == HAL_OK) {
        schedule_mgr.lin_state = LIN_HEADER_TX;
        schedule_mgr.lin_result = LIN_PENDING;
    } else {
        schedule_mgr.lin_state = LIN_IDLE;
        schedule_mgr.lin_result = LIN_ERROR;
    }

    return status;
}

uint8_t LIN_calculate_checksum(uint8_t pID, uint8_t *data, uint8_t data_length) {
    uint16_t sum = 0;
    uint8_t frame_id = pID & 0x3FU;

    if (frame_id != 0x3CU && frame_id != 0x3DU) {
        sum = pID;
    }
    
    for (uint8_t i = 0; i < data_length; ++i) {
        sum += data[i];
        if (sum > 0xFF) {
            sum -= 0xFF;
        }
    }

    return (uint8_t)(~sum);
}

uint8_t LIN_calculate_PID(uint8_t frameID) {
    uint8_t p0 = (frameID ^ (frameID >> 1) ^ (frameID >> 2) ^ (frameID >> 4)) & 1U;
    uint8_t p1 = (~((frameID >> 1) ^ (frameID >> 3) ^ (frameID >> 4) ^ (frameID >> 5))) & 1U;

    return (p1 << 7) | (p0 << 6) | frameID;
}

uint32_t LIN_calculate_header_transmission_time_us(void) {
    return 34 * LIN_TBIT_US;
}

uint32_t LIN_calculate_data_transmission_time_us(const LIN_ScheduleSlot_t *slot) {
    if (slot == NULL) {
        return 0U;
    }

    return (slot->data_length + 1) * 10 * LIN_TBIT_US;
}

uint32_t LIN_calculate_slot_duration_ms(const LIN_ScheduleSlot_t *slot) {
    if (slot == NULL) {
        return 0U;
    }
    uint32_t t_header_us = LIN_calculate_header_transmission_time_us();
    uint32_t t_response_us = LIN_calculate_data_transmission_time_us(slot);

    uint64_t total_us = ((uint64_t)t_header_us + t_response_us) * 14U;
    total_us = (total_us + 9U) / 10U;

    const uint32_t timebase_us = LIN_TIMEBASE_MS * 1000U;

    return (uint32_t)((total_us + timebase_us - 1U) / timebase_us) * LIN_TIMEBASE_MS;
}

void delay_us(TIM_HandleTypeDef *htim, uint16_t duration) {
    uint16_t start_time_us = __HAL_TIM_GET_COUNTER(htim);

    while ((uint16_t)(__HAL_TIM_GET_COUNTER(htim) - start_time_us) < duration) {
    }
}

void LIN_HAL_TIM_Callback(void) {
    if (schedule_mgr.table == NULL || schedule_mgr.total_slots == 0U) {
        return;
    }

    if (!schedule_mgr.started_schedule) {
        schedule_mgr.started_schedule = true;
        schedule_mgr.active_slot_idx = schedule_mgr.schedule_slot_idx;
        schedule_mgr.elapsed_ms = 0U;

        if (LIN_execute_slot(schedule_mgr.huart) != HAL_OK) {
            // TODO: Handle failed start
        }
        return;
    }

    schedule_mgr.elapsed_ms += LIN_TIMEBASE_MS;
    uint32_t duration_ms = schedule_mgr.table[schedule_mgr.active_slot_idx].duration_ms;

    if (schedule_mgr.elapsed_ms < duration_ms) {
        return;
    }

    if (schedule_mgr.lin_state != LIN_IDLE && schedule_mgr.lin_result == LIN_PENDING) {
        schedule_mgr.lin_result = LIN_TIMEOUT;
    }
    
    schedule_mgr.schedule_slot_idx = (schedule_mgr.active_slot_idx + 1U) % schedule_mgr.total_slots;

    if (schedule_mgr.lin_result == LIN_SUCCESS) {
        schedule_mgr.active_slot_idx = schedule_mgr.schedule_slot_idx;
        schedule_mgr.elapsed_ms = 0U;
        schedule_mgr.lin_result = LIN_PENDING;

        if (LIN_execute_slot(schedule_mgr.huart) != HAL_OK) {
            // TODO: Handle failed start
        }
        return;
    }
    else if (schedule_mgr.lin_result == LIN_TIMEOUT) {
        // TODO: Handle timeout
    }
    else {
        // TODO: Handle error
    }
}

void LIN_HAL_UART_Tx_Callback(UART_HandleTypeDef *huart) {
    if (huart != schedule_mgr.huart) {
        return;
    }

    if (huart->ErrorCode == HAL_UART_ERROR_NONE) {
        if (schedule_mgr.lin_state == LIN_HEADER_TX) {
            uint8_t slot_idx = schedule_mgr.active_slot_idx;
            LIN_ScheduleSlot_t *slot = &schedule_mgr.table[slot_idx];

            if (slot->direction == MASTER_SUBSCRIBE) {
                HAL_StatusTypeDef status = HAL_UART_Receive_DMA(
                    huart,
                    schedule_mgr.buffer,
                    slot->data_length + 3);

                if (status == HAL_OK) {
                    schedule_mgr.lin_state = LIN_SLAVE_RESPONSE_RX;
                } else {
                    schedule_mgr.lin_state = LIN_RECOVERY;
                    schedule_mgr.lin_result = LIN_ERROR;
                }
            } else {
                HAL_StatusTypeDef status = LIN_transmit_data(huart);

                if (status == HAL_OK) {
                    schedule_mgr.lin_state = LIN_MASTER_RESPONSE_TX;
                } else {
                    schedule_mgr.lin_state = LIN_RECOVERY;
                    schedule_mgr.lin_result = LIN_ERROR;
                }
            }
        } else if (schedule_mgr.lin_state == LIN_MASTER_RESPONSE_TX) {
            schedule_mgr.lin_state = LIN_IDLE;
            schedule_mgr.lin_result = LIN_SUCCESS;
        }
    } else {
        schedule_mgr.lin_state = LIN_RECOVERY;
        schedule_mgr.lin_result = LIN_ERROR;
    }
}

void LIN_HAL_UART_Rx_Callback(UART_HandleTypeDef *huart) {
    if (huart != schedule_mgr.huart) {
        return;
    }

    if (huart->ErrorCode == HAL_UART_ERROR_NONE) {
        uint8_t slot_idx = schedule_mgr.active_slot_idx;
        LIN_ScheduleSlot_t *slot = &schedule_mgr.table[slot_idx];

        if (slot->direction == MASTER_SUBSCRIBE) {
            uint8_t checksum = schedule_mgr.buffer[slot->data_length + 2]; 

            bool is_valid_data = (LIN_calculate_checksum(
                slot->pID,
                schedule_mgr.buffer + 2,
                slot->data_length) == checksum);
            
            if (is_valid_data && slot->data != NULL) {
                memcpy(slot->data, schedule_mgr.buffer, slot->data_length);
                schedule_mgr.lin_result = LIN_SUCCESS;
            } else {
                schedule_mgr.lin_result = LIN_ERROR;
            }
        }

        schedule_mgr.lin_state = LIN_IDLE;
    } else {
        schedule_mgr.lin_state = LIN_RECOVERY;
        schedule_mgr.lin_result = LIN_ERROR;
    }
}