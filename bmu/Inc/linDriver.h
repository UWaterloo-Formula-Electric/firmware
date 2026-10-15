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
#define LIN_BUFFER_FACTOR 14
#define LIN_MAX_RETRIES 2U


typedef enum {
    MASTER_PUBLISH,
    MASTER_SUBSCRIBE
} LIN_Direction_t;

typedef enum {
    FRAME_UNCONDITIONAL,
    FRAME_EVENT_TRIGGERED,
    FRAME_DIGNOSTIC
} LIN_FrameType_t;

typedef enum {
    LIN_IDLE,
    LIN_READY,
    LIN_TX_HEADER,
    LIN_TX_RESPONSE,
    LIN_RX_RESPONSE,
    LIN_RECOVERY
} LIN_State_t;

typedef enum {
    LIN_PENDING,
    LIN_SUCCESS,
    LIN_TIMEOUT,
    LIN_ERROR
} LIN_Result_t;

typedef struct {
    uint8_t header_bytes[2];
    uint8_t response_bytes[LIN_MAX_DATA_SIZE + 1];
    uint8_t response_length;
} LIN_Frame_t;

typedef struct {
    uint8_t duration_ms;
    LIN_Direction_t direction;
    LIN_FrameType_t frame_type;
    LIN_Frame_t *frame;
} LIN_ScheduleSlot_t;

typedef struct {
    LIN_ScheduleSlot_t *table;
    uint8_t n_slots;
    uint8_t schedule_slot_idx;
    uint8_t active_slot_idx;
    uint32_t elapsed_ms;
} LIN_ScheduleManager_t;

typedef struct {
    TIM_HandleTypeDef *htim;
    UART_HandleTypeDef *huart;
} LIN_Config_t;

typedef struct {
    LIN_Config_t config;
    LIN_ScheduleManager_t schedule_manager;
    volatile LIN_State_t frame_state;
    volatile LIN_Result_t frame_result;
} LIN_HandleTypeDef;

HAL_StatusTypeDef LIN_init_frame(
    LIN_Frame_t *frame,
    uint8_t frameID,
    uint8_t *response_bytes,
    uint8_t response_length
);
HAL_StatusTypeDef LIN_init_slot(
    LIN_ScheduleSlot_t *slot,
    LIN_Direction_t direction,
    LIN_FrameType_t frame_type,
    LIN_Frame_t *frame
);

HAL_StatusTypeDef LIN_init_schedule(
    LIN_ScheduleManager_t *schedule_manager,
    LIN_ScheduleSlot_t *table,
    uint8_t n_slots
);

LIN_ScheduleSlot_t* get_active_slot();

HAL_StatusTypeDef LIN_wakeup(UART_HandleTypeDef *huart);
HAL_StatusTypeDef LIN_transmit_header(TIM_HandleTypeDef *htim, UART_HandleTypeDef *huart, const LIN_Frame_t *frame);
HAL_StatusTypeDef LIN_transmit_response(UART_HandleTypeDef *huart, const LIN_Frame_t *frame);

uint8_t LIN_calculate_checksum(
    const uint8_t pID,
    const uint8_t *response_bytes,
    const uint8_t response_length
);
uint8_t LIN_calculate_PID(const uint8_t frameID);
uint32_t LIN_calculate_header_transmission_time_us(void);
uint32_t LIN_calculate_response_transmission_time_us(const uint8_t response_length);
uint32_t LIN_calculate_slot_duration_ms(const uint8_t response_length);

void LIN_start_schedule(TIM_HandleTypeDef *htim, UART_HandleTypeDef *huart);
void LIN_HAL_TIM_Callback(void);
void LIN_HAL_UART_Tx_Callback(UART_HandleTypeDef *huart);
void LIN_HAL_UART_Rx_Callback(UART_HandleTypeDef *huart);

#endif /* end of include guard: LIN_DRIVER_H */