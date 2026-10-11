#ifndef LIN_DRIVER_H
#define LIN_DRIVER_H

#include "stm32f7xx_hal.h"
#include "stdint.h"

#define TLIN1029Q1_LIN_BUS_WAKEUP_US 65U
#define TLIN1029Q1_DOMINANT_STATE_TIMEOUT_MS 45U
#define TLIN1029Q1_NORMAL_MODE_INIT_US 35U
#define TLIN1029Q1_POWERUP_MS 2U
#define TLIN1029Q1_MAX_RX_PROPOGATION_DELAY_US 6U

#define LIN_SYNC_BYTE 0x55U
#define LIN_TBIT_US 50
#define LIN_TIMEBASE_MS 5U
#define LIN_MAX_DATA_SIZE 8U

typedef enum {
    LIN_CHECKSUM_CLASSIC,
    LIN_CHECKSUM_ENHANCED
} LIN_ChecksumType_t;

typedef enum {
    LIN_MASTER_PUBLISH,
    LIN_MASTER_SUBSCRIBE
} LIN_Direction_t;

typedef struct {
    uint8_t pID;
    uint8_t *data;
    uint8_t data_length;
    uint16_t duration_ms;
    LIN_Direction_t direction;
    LIN_ChecksumType_t cks_type;
} LIN_ScheduleSlot_t;

typedef struct {
    LIN_ScheduleSlot_t *table;
    uint8_t total_slots;
    uint8_t current_slot_idx;
    uint8_t buffer[LIN_MAX_DATA_SIZE + 3];
} LIN_ScheduleManager_t;

HAL_StatusTypeDef LIN_init_slot(
    LIN_ScheduleSlot_t *slot,
    uint8_t frameID,
    uint8_t data_length,
    LIN_Direction_t direction,
    LIN_ChecksumType_t cks_type,
    uint8_t *data
);
HAL_StatusTypeDef LIN_init_schedule(
    LIN_ScheduleManager_t *schedule_mgr,
    LIN_ScheduleSlot_t *table,
    uint8_t size
);
HAL_StatusTypeDef LIN_wakeup_bus(
    GPIO_TypeDef *portType,
    uint16_t portNum,
    TIM_HandleTypeDef *htim
);
HAL_StatusTypeDef LIN_transmit_header(
    UART_HandleTypeDef *huart,
    LIN_ScheduleManager_t *schedule_mgr
);
HAL_StatusTypeDef LIN_transmit_data(
    UART_HandleTypeDef *huart,
    LIN_ScheduleManager_t *schedule_mgr
);
HAL_StatusTypeDef LIN_transmit_frame(
    UART_HandleTypeDef *huart,
    LIN_ScheduleManager_t *schedule_mgr
);
HAL_StatusTypeDef LIN_receive_frame(
    UART_HandleTypeDef *huart,
    LIN_ScheduleManager_t *schedule_mgr
);
HAL_StatusTypeDef LIN_validate_checksum(
    uint8_t pID,
    uint8_t *data,
    uint8_t data_length,
    uint8_t checksum
);
HAL_StatusTypeDef LIN_execute_schedule(
    UART_HandleTypeDef *huart,
    LIN_ScheduleManager_t *schedule_mgr
);

uint8_t LIN_calculate_checksum(uint8_t pID, uint8_t *data, uint8_t data_length);
uint8_t LIN_calculate_PID(uint8_t frameID);
uint16_t LIN_calculate_duration_ms(LIN_ScheduleSlot_t *slot);
void delay_us(TIM_HandleTypeDef *htim, uint8_t duration);
void LIN_HAL_TIM_Callback(LIN_ScheduleManager_t *schedule_mgr);

#endif /* end of include guard: LIN_DRIVER_H */