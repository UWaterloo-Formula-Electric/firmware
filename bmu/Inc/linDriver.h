#ifndef LIN_DRIVER_H
#define LIN_DRIVER_H

#include "stm32f7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

/* LIN 2.x protocol constants */
#define LIN_TIMEBASE_US 5000U
#define LIN_TIME_BUFFER_PCT 140U
#define LIN_BREAK_BITS 14U
#define LIN_HEADER_BITS 34U
#define LIN_SYNC_BYTE 0x55U
#define LIN_BITS_PER_BYTE 10U
#define LIN_MAX_DATA_SIZE 8U
#define LIN_MAX_BAUD 20000U
#define LIN_MIN_BAUD 2400U
#define LIN_MAX_FRAME_ID 0x3DU
#define LIN_DIAG_MASTER_REQUEST_FRAME 0x3CU
#define LIN_DIAG_SLAVE_RESPONSE_FRAME 0x3DU

/* LIN schedule table constants */
#define LIN_MAX_FRAMES 16U
#define LIN_MAX_SLOTS 16U

/* LIN driver constants */
#define LIN_DMA_BUFFER_SIZE 256U

/* TLIN1029-Q1 constants */
#define TLIN1029Q1_WAKEUP_US 65U
#define TLIN1029Q1_CLEAR_FALSE_WAKEUP_US 25U

typedef enum {
    LIN_FRAME_UNCONDITIONAL,
    LIN_FRAME_EVENT_TRIGGERED,
    LIN_FRAME_SPORADIC,
    LIN_FRAME_DIAGNOSTIC
} LIN_FrameType_t;

typedef enum {
    LIN_CHECKSUM_CLASSIC,
    LIN_CHECKSUM_ENHANCED
} LIN_ChecksumType_t;

typedef enum {
    LIN_MASTER_PUBLISH,
    LIN_MASTER_SUBSCRIBE
} LIN_Direction_t;

/* LIN 2.x master task state machine */
typedef enum {
    LIN_MASTER_IDLE,
    LIN_MASTER_TX_BREAK,
    LIN_MASTER_TX_HEADER,
    LIN_MASTER_TX_RESPONSE,
    LIN_MASTER_RX_RESPONSE,
    LIN_MASTER_SUCCESS,
    LIN_MASTER_ERROR
} LIN_MasterState_t;

typedef struct {
    LIN_MasterState_t master_state;
    LIN_Direction_t direction;
} LIN_Transaction_t;

typedef struct {
    uint8_t id;
    uint8_t pid;
    // Out of sync with tx_bytes if tx_updated is false
    uint8_t length;
    LIN_Direction_t direction;
    LIN_FrameType_t frame_type;
    LIN_ChecksumType_t checksum_type;
    uint8_t header_bytes[2];
    uint8_t response_bytes[LIN_MAX_DATA_SIZE + 1];
    uint8_t update_buffer[LIN_MAX_DATA_SIZE];
    bool rx_valid_response;
    bool tx_update_pending;
} LIN_Frame_t;

typedef struct {
    uint32_t duration_us;
    uint8_t frame_index;
} LIN_Slot_t;

typedef struct {
    LIN_Frame_t frames[LIN_MAX_FRAMES];
    LIN_Slot_t slots[LIN_MAX_SLOTS];
    uint8_t frame_count;
    uint8_t slot_count;
    uint8_t slot_index;
    uint32_t elapsed_us;
} LIN_Schedule_t;

typedef struct {
    TIM_HandleTypeDef *htim;
    UART_HandleTypeDef *huart;
    LIN_Schedule_t schedule;
    LIN_Transaction_t transaction;
    uint8_t dma_buffer[LIN_DMA_BUFFER_SIZE];
    uint16_t dma_index;
    uint8_t echo_bytes[LIN_MAX_DATA_SIZE + 3];
    uint8_t echo_size;
    bool is_running;
} LIN_Driver_t;

/* Initializers */
HAL_StatusTypeDef LIN_init_frame(
    LIN_Frame_t *frame,
    uint8_t id,
    uint8_t length,
    LIN_Direction_t direction,
    LIN_FrameType_t frame_type
);
HAL_StatusTypeDef LIN_init_schedule(
    LIN_Schedule_t *schedule,
    LIN_Frame_t *frames,
    uint8_t frame_count,
    LIN_Slot_t *slots,
    uint8_t slot_count
);
HAL_StatusTypeDef LIN_init_driver(
    LIN_Driver_t *driver,
    LIN_Schedule_t *schedule,
    TIM_HandleTypeDef *htim,
    UART_HandleTypeDef *huart
);

/* LIN 2.x protocol helpers */
HAL_StatusTypeDef LIN_make_protected_ID(uint8_t id, uint8_t *pid);
HAL_StatusTypeDef LIN_calculate_checksum(
    LIN_ChecksumType_t checksum_type,
    const uint8_t pid,
    const uint8_t length,
    const uint8_t *bytes,
    uint8_t *checksum
);
HAL_StatusTypeDef LIN_update_frame(
    LIN_Schedule_t *schedule,
    uint8_t index,
    uint8_t *data,
    uint8_t length);
uint32_t LIN_calculate_nominal_t_transmission_us(
    const uint32_t baudrate,
    const uint8_t length
);
uint32_t LIN_calculate_max_t_transmission_us(
    const uint32_t baudrate,
    const uint8_t length
);
uint32_t LIN_round_t_tranmission_to_timebase_us(const uint32_t t_us);

/* LIN 2.x state machines */
void LIN_master_state_machine(
    LIN_Transaction_t *transaction,
    const bool is_complete
);

/* Transmission functions*/
void LIN_read_dma_buffer(LIN_Driver_t *driver);

/* Callback overrides */
void LIN_HAL_TIM_PeriodElapsedCallback(
    LIN_Driver_t *driver,
    TIM_HandleTypeDef *htim
);

void LIN_HAL_UARTEx_RxEventCallback(
    LIN_Driver_t *driver,
    UART_HandleTypeDef *huart,
    uint16_t Size
);
void LIN_HAL_UART_IRQHandler(
    LIN_Driver_t *driver,
    UART_HandleTypeDef *huart
);
void LIN_HAL_UART_ErrorCallback(
    LIN_Driver_t *driver,
    UART_HandleTypeDef *huart
);

#endif /* LIN_DRIVER_H */