#ifndef LIN_DRIVER_H
#define LIN_DRIVER_H

#include "stm32f7xx_hal.h"
#include "stdint.h"
#include "stdbool.h"

#define TLIN1029Q1_LIN_BUS_WAKEUP_US 65U
#define TLIN1029Q1_DOMINANT_STATE_TIMEOUT_MS 45U
#define TLIN1029Q1_NORMAL_MODE_INIT_US 35U
#define TLIN1029Q1_POWERUP_MS 2U
#define TLIN1029Q1_MAX_RX_PROPOGATION_DELAY_US 6U

#define LIN_SYNC_BYTE 0x55U
#define LIN_TBIT_US 50
#define LIN_TIMEBASE_MS 5U
#define LIN_MAX_DATA_SIZE 8U
#define LIN_SLOT_BUFFER_FACTOR 1.4f
#define LIN_MAX_RETRIES 2U

typedef enum {
    MASTER_PUBLISH,
    MASTER_SUBSCRIBE
} LIN_Direction_t;

typedef enum {
    LIN_ASLEEP,
    LIN_IDLE,
    LIN_HEADER_TX,
    LIN_MASTER_RESPONSE_TX,
    LIN_SLAVE_RESPONSE_RX,
    LIN_RECOVERY
} LIN_State_t;

typedef enum {
    LIN_PENDING,
    LIN_SUCCESS,
    LIN_TIMEOUT,
    LIN_ERROR
} LIN_Result_t;

typedef struct {
    uint8_t n_retries;
    uint8_t pID;
    uint8_t *data;
    uint8_t data_length;
    uint32_t duration_ms;
    LIN_Direction_t direction;
} LIN_ScheduleSlot_t;

typedef struct {
    UART_HandleTypeDef *huart;
    LIN_ScheduleSlot_t *table;
    LIN_State_t lin_state;
    LIN_Result_t lin_result;
    bool started_schedule;
    uint32_t elapsed_ms;
    uint8_t total_slots;
    uint8_t schedule_slot_idx;
    uint8_t active_slot_idx;
    uint8_t buffer[LIN_MAX_DATA_SIZE + 3];
} LIN_ScheduleManager_t;

extern LIN_ScheduleManager_t schedule_mgr;

HAL_StatusTypeDef LIN_init_slot(
    LIN_ScheduleSlot_t *slot,
    uint8_t frameID,
    uint8_t data_length,
    LIN_Direction_t direction,
    uint8_t *data
);
HAL_StatusTypeDef LIN_init_schedule(
    UART_HandleTypeDef *huart,
    LIN_ScheduleSlot_t *table,
    uint8_t total_slots
);
HAL_StatusTypeDef LIN_wakeup_bus(
    GPIO_TypeDef *portType,
    uint16_t portNum,
    TIM_HandleTypeDef *htim
);
HAL_StatusTypeDef LIN_transmit_header(UART_HandleTypeDef *huart);
HAL_StatusTypeDef LIN_transmit_data(UART_HandleTypeDef *huart);

HAL_StatusTypeDef LIN_execute_slot(UART_HandleTypeDef *huart);

uint8_t LIN_calculate_checksum(uint8_t pID, uint8_t *data, uint8_t data_length);
uint8_t LIN_calculate_PID(uint8_t frameID);
uint32_t LIN_calculate_header_transmission_time_us();
uint32_t LIN_calculate_data_transmission_time_us(const LIN_ScheduleSlot_t *slot);
uint32_t LIN_calculate_slot_duration_ms(const LIN_ScheduleSlot_t *slot);
void delay_us(TIM_HandleTypeDef *htim, uint16_t duration);

void LIN_HAL_TIM_Callback();
void LIN_HAL_UART_Tx_Callback(UART_HandleTypeDef *huart);
void LIN_HAL_UART_Rx_Callback(UART_HandleTypeDef *huart);

#endif /* end of include guard: LIN_DRIVER_H */