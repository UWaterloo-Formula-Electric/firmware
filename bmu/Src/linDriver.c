#include "linDriver.h"

HAL_StatusTypeDef LIN_init_frame(
    LIN_Frame_t *frame,
    uint8_t id,
    uint8_t length,
    LIN_Direction_t direction,
    LIN_FrameType_t frame_type) {
    if (frame == NULL || length == 0 || id > LIN_MAX_FRAME_ID) {
        return HAL_ERROR;
    }

    frame->id = id;
    
    frame->pid = frame->id;
    LIN_make_protected_ID(&(frame->pid));
    
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

    frame->rx_valid = false;
    frame->tx_updated = false;

    memset(frame->tx_bytes, 0, LIN_MAX_DATA_SIZE + 1);
    memset(frame->rx_bytes, 0, LIN_MAX_DATA_SIZE + 1);

    return HAL_OK;
}

HAL_StatusTypeDef LIN_init_schedule(
    LIN_Schedule_t *schedule,
    LIN_Frame_t *frames,
    uint16_t frame_count,
    LIN_Slot_t *slots,
    uint16_t slot_count) {
    if (schedule == NULL || frames == NULL || slots == NULL ||
        frame_count == 0 || frame_count > LIN_MAX_FRAMES ||
        slot_count == 0 || slot_count > LIN_MAX_SLOTS) {
        return HAL_ERROR;
    }

    memcpy(schedule->frames, frames, frame_count);
    memcpy(schedule->slots, slots, slot_count);

    schedule->frame_count = frame_count;
    schedule->slot_count = slot_count;
    schedule->slot_index = 0;
    schedule->elapsed_us = 0;

    return HAL_OK;
}

HAL_StatusTypeDef LIN_init_driver(
    LIN_Driver_t *driver,
    LIN_Schedule_t *schedule,
    UART_HandleTypeDef *huart
) {
    if (driver == NULL || schedule == NULL || huart == NULL ||
        schedule->frame_count == 0 || schedule->slot_count == 0) {
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

    memset(driver, 0, sizeof(LIN_Driver_t));
    driver->schedule = *schedule;
    driver->huart = huart;
    // Arm circular RX DMA
    HAL_StatusTypeDef circular_dma_status = HAL_UARTEx_ReceiveToIdle_DMA(
        driver->huart,
        driver->circ_buffer,
        LIN_CIRC_BUFFER_SIZE
    );
    
    if (circular_dma_status != HAL_OK) {
        return circular_dma_status;
    }

    // Stop framing errors from breaking echo
    CLEAR_BIT(huart->Instance->CR3, USART_CR3_EIE);

    // Enable LIN Break Detection (LBD) interrupt
    __HAL_UART_ENABLE_IT(huart, UART_IT_LBD);

    return HAL_OK;
}

HAL_StatusTypeDef LIN_make_protected_ID(uint8_t *p_id) {
    if (p_id == NULL || *p_id > LIN_MAX_FRAME_ID) return HAL_ERROR;

    uint8_t po = ((*p_id) ^ (*p_id >> 1U) ^ (*p_id >> 2U) ^ (*p_id >> 4U)) & 1U;
    uint8_t p1 = (~((*p_id >> 1) ^ (*p_id >> 3U) ^ (*p_id >> 4) ^ (*p_id >> 5))) & 1U;

    *p_id = *p_id | (po << 6U) | (p1 << 7U);

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
    if (index >= LIN_MAX_FRAMES || data == NULL ||
        length == 0 || length > LIN_MAX_DATA_SIZE) {
        return HAL_ERROR;
    }

    LIN_Frame_t *frame = &(schedule->frames[index]);

    if (frame->direction == LIN_MASTER_SUBSCRIBE) {
        return HAL_ERROR;
    }
    
    frame->tx_updated = false;
    memcpy(frame->rx_bytes, data, length);
    frame->length = length;

    return HAL_OK;
}

uint32_t LIN_calculate_nominal_t_transmission_us(
    const uint32_t baudrate,
    const uint8_t length) {
    uint32_t t_bit_us = 1e6 / baudrate;
    return length * t_bit_us * LIN_BITS_PER_BYTE;
}

uint32_t LIN_calculate_max_t_transmission_us(
    const uint32_t baudrate,
    const uint8_t length) {
    uint32_t t_trx_us = LIN_calculate_nominal_t_transmission_us(baudrate, length);
    return (t_trx_us * LIN_TIME_BUFFER_PCT + 50) / 100;
}

uint32_t LIN_round_t_tranmission_to_timebase_us(const uint32_t t_transmission_us) {
    if (t_transmission_us % LIN_TIMEBASE_US > 0) {
        return t_transmission_us + t_transmission_us % LIN_TIMEBASE_US;
    }
    return t_transmission_us;
}

LIN_MasterState_t LIN_master_state_machine(
    const LIN_MasterState_t master_state,
    const HAL_StatusTypeDef dma_status
) {
    LIN_MasterState_t next_state;

    // LIN 2.x signal transmitted before frame is sent
    switch (master_state) {
        case LIN_MASTER_IDLE:
            if (dma_status == HAL_OK) {
                next_state = LIN_MASTER_TX_BREAK;
            }
            else {
                next_state = LIN_MASTER_ERROR;
            }
            break;
        case LIN_MASTER_TX_BREAK:
            if (dma_status == HAL_OK) {
                next_state = LIN_MASTER_TX_SYNC;
            }
            else {
                next_state = LIN_MASTER_ERROR;
            }
            break;
        case LIN_MASTER_TX_SYNC:
            if (dma_status == HAL_OK) {
                next_state = LIN_MASTER_TX_PID;
            }
            else {
                next_state = LIN_MASTER_ERROR;
            }
            break;
        case LIN_MASTER_TX_PID:
            if (dma_status == HAL_OK) {
                next_state = LIN_MASTER_IDLE;
            }
            else {
                next_state = LIN_MASTER_ERROR;
            }
            break;
        case LIN_MASTER_ERROR:
            if (dma_status == HAL_OK) {
                next_state = LIN_MASTER_IDLE;
            }
            else {
                next_state = LIN_MASTER_ERROR;
            }
            break;
        default:
            next_state = master_state;
            break;
    }

    return next_state;
}

LIN_SlaveState_t LIN_slave_state_machine(
    const LIN_SlaveState_t slave_state,
    const HAL_StatusTypeDef validation_status) {
    LIN_SlaveState_t next_state;
    
    // LIN 2.x signal received when checksum is valid
    switch (slave_state) {
        case LIN_SLAVE_IDLE:
            if (validation_status == HAL_OK) {
                next_state = LIN_SLAVE_RX_SYNC;
            }
            else {
                next_state = LIN_SLAVE_ERROR;
            }
            break;
        case LIN_SLAVE_RX_SYNC:
            if (validation_status == HAL_OK) {
                next_state = LIN_SLAVE_RX_PID;
            }
            else {
                next_state = LIN_SLAVE_ERROR;
            }
            break;
        case LIN_SLAVE_RX_PID:
            if (validation_status == HAL_OK) {
                next_state = LIN_SLAVE_RX_DATA;
            }
            else {
                next_state = LIN_SLAVE_ERROR;
            }
            break;
        case LIN_SLAVE_RX_DATA:
            if (validation_status == HAL_OK) {
                next_state = LIN_SLAVE_RX_CHECKSUM;
            }
            else {
                next_state = LIN_SLAVE_ERROR;
            }
            break;
        case LIN_SLAVE_RX_CHECKSUM:
            if (validation_status == HAL_OK) {
                next_state = LIN_SLAVE_IDLE;
            }
            else {
                next_state = LIN_SLAVE_ERROR;
            }
            break;
        case LIN_SLAVE_TX_DATA:
            if (validation_status == HAL_OK) {
                next_state = LIN_SLAVE_TX_CHECKSUM;
            }
            else {
                next_state = LIN_SLAVE_ERROR;
            }
            break;
        case LIN_SLAVE_TX_CHECKSUM:
            if (validation_status == HAL_OK) {
                next_state = LIN_SLAVE_IDLE;
            }
            else {
                next_state = LIN_SLAVE_ERROR;
            }
            break;
        case LIN_SLAVE_ERROR:
            if (validation_status == HAL_OK) {
                next_state = LIN_SLAVE_IDLE;
            }
            else {
                next_state = LIN_SLAVE_ERROR;
            }
            break;
        default:
            next_state = slave_state;
            break;
    }

    return next_state;
}

HAL_StatusTypeDef LIN_send_break(UART_HandleTypeDef *huart) {
    if (huart == NULL) return HAL_ERROR;

    return HAL_LIN_SendBreak(huart);
}

HAL_StatusTypeDef LIN_send_bytes(
    UART_HandleTypeDef *huart,
    uint8_t *data,
    uint8_t length) {
    if (huart == NULL || data == NULL ||
        length == 0 || length > LIN_MAX_DATA_SIZE) {
        return HAL_ERROR;
    }

    return HAL_UART_Transmit_DMA(huart, data, length);
}