#ifndef LIN_DRIVER_H
#define LIN_DRIVER_H

#include "stm32f7xx_hal.h"
#include "stdint.h"

#define TLIN1029Q1_LIN_BUS_WAKEUP_US 65U
#define TLIN1029Q1_CLEAR_US 25U
#define TLIN1029Q1_DOMINANT_STATE_TIMEOUT_MS 45U
#define TLIN1029Q1_NORMAL_MODE_INIT_US 35U
#define TLIN1029Q1_POWERUP_MS 2U

#define LIN_SYNC_BYTE 0x55U
#define LIN_MAX_DATA_SIZE 8U
#define LIN_TIMEBASE_MS 5U

typedef enum {
    LIN_CHECKSUM_CLASSIC,
    LIN_CHECKSUM_ENHANCED
} LIN_ChecksumType_t;

typedef enum {
    LIN_MASTER_PUBLISH,
    LIN_MASTER_SUBSCRIBE
} LIN_Direction_t;

typedef struct {
    uint8_t pid;
    uint8_t data_length;
    uint16_t duration_ms;
    LIN_Direction_t direction;
    LIN_ChecksumType_t cks_type;
    uint8_t *p_data_buffer;
} LIN_ScheduleSlot_t;

typedef struct {
    LIN_ScheduleSlot_t *table;
    uint8_t total_slots;
    uint8_t current_slot_idx;
} LIN_ScheduleManager_t;

typedef struct {
    uint8_t pid;
    uint8_t data_length;
    uint8_t data[LIN_MAX_DATA_SIZE];
    uint8_t checksum;
} LIN_Frame_t;

HAL_StatusTypeDef LIN_init_slot(
    LIN_ScheduleSlot_t *slot,
    uint8_t frameID,
    uint8_t data_length,
    uint16_t duration_ms,
    LIN_Direction_t direction,
    LIN_ChecksumType_t cks_type,
    uint8_t *p_data_buffer
);
HAL_StatusTypeDef LIN_init_schedule(
    LIN_ScheduleManager_t *schedule_mgr,
    LIN_ScheduleSlot_t *table,
    uint8_t size
);
HAL_StatusTypeDef LIN_init_frame(
    LIN_Frame_t *frame,
    LIN_ScheduleSlot_t *slot
);

HAL_StatusTypeDef LIN_wakeup_bus(
    GPIO_TypeDef *portType,
    uint16_t portNum,
    TIM_HandleTypeDef *htim
);

HAL_StatusTypeDef LIN_transmit_frame(UART_HandleTypeDef *huart, const LIN_Frame_t *frame);
HAL_StatusTypeDef LIN_receive_frame(UART_HandleTypeDef *huart, LIN_Frame_t* frame);
HAL_StatusTypeDef LIN_validate_checksum(const LIN_Frame_t *frame);

HAL_StatusTypeDef LIN_execute_schedule(UART_HandleTypeDef *huart, LIN_ScheduleManager_t *schedule_mgr);
void LIN_HAL_TIM_Callback(LIN_ScheduleManager_t *schedule_mgr);

uint8_t LIN_calculate_checksum(LIN_Frame_t *frame);
uint8_t LIN_calculate_PID(uint8_t frameID);
void delay_us(TIM_HandleTypeDef *htim, uint8_t duration);
void flush_rdx_buffer(UART_HandleTypeDef *huart);

#endif /* end of include guard: LIN_DRIVER_H */