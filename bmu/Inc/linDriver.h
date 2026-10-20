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
#define LIN_CIRC_BUFFER_SIZE 256U

/* TLIN1029-Q1 constants */
#define TLIN1029Q1_WAKEUP_US 65U
#define TLIN1029Q1_CLEAR_FALSE_WAKEUP_US 25U
#define TLIN1029Q1_DOMINANT_STATE_TIMEOUT_US 45000U

typedef enum {
    LIN_MASTER_PUBLISH,
    LIN_MASTER_SUBSCRIBE
} LIN_Direction_t;

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

/* LIN 2.x master task state machine */
typedef enum {
    LIN_MASTER_IDLE,
    LIN_MASTER_TX_BREAK,
    LIN_MASTER_TX_SYNC,
    LIN_MASTER_TX_PID,
    LIN_MASTER_ERROR
} LIN_MasterState_t;

/* LIN 2.x slave task state machine */
typedef enum {
    LIN_SLAVE_IDLE,
    LIN_SLAVE_RX_SYNC,
    LIN_SLAVE_RX_PID,
    LIN_SLAVE_RX_DATA,
    LIN_SLAVE_RX_CHECKSUM,
    LIN_SLAVE_TX_DATA,
    LIN_SLAVE_TX_CHECKSUM,
    LIN_SLAVE_ERROR
} LIN_SlaveState_t;

typedef struct {
    LIN_MasterState_t master_state;
    LIN_SlaveState_t slave_state;
} LIN_FrameSM_t;

typedef struct {
    uint8_t id;
    uint8_t pid;
    // Out of sync with tx_bytes if tx_updated is false
    uint8_t length;
    LIN_Direction_t direction;
    LIN_FrameType_t frame_type;
    LIN_ChecksumType_t checksum_type;
    uint8_t tx_bytes[LIN_MAX_DATA_SIZE + 1];
    // Update buffer if direction is LIN_MASTER_PUBLISH
    uint8_t rx_bytes[LIN_MAX_DATA_SIZE + 1];
    bool rx_valid;
    bool tx_updated;
} LIN_Frame_t;

typedef struct {
    uint32_t duration_us;
    uint8_t frame_index;
} LIN_Slot_t;

typedef struct {
    LIN_Frame_t frames[LIN_MAX_FRAMES];
    LIN_Slot_t slots[LIN_MAX_SLOTS];
    uint16_t frame_count;
    uint16_t slot_count;
    uint16_t slot_index;
    uint32_t elapsed_us;
} LIN_Schedule_t;

typedef struct {
    UART_HandleTypeDef *huart;
    LIN_Schedule_t schedule;
    LIN_FrameSM_t frame_processor;
    uint8_t circ_buffer[LIN_CIRC_BUFFER_SIZE];
    uint16_t circ_buffer_index;
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
    uint16_t frame_count,
    LIN_Slot_t *slots,
    uint16_t slot_count
);
HAL_StatusTypeDef LIN_init_driver(
    LIN_Driver_t *driver,
    LIN_Schedule_t *schedule,
    UART_HandleTypeDef *huart
);

/* LIN 2.x protocol helpers */
HAL_StatusTypeDef LIN_make_protected_ID(uint8_t *pid);
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
uint32_t LIN_round_t_tranmission_to_timebase_us(const uint32_t t_transmission_us);

/* LIN 2.x state machines */
LIN_MasterState_t LIN_master_state_machine(
    const LIN_MasterState_t master_state,
    const HAL_StatusTypeDef dma_status
);
LIN_SlaveState_t LIN_slave_state_machine(
    const LIN_SlaveState_t slave_state,
    const HAL_StatusTypeDef validation_status
);

/* Transmission functions*/
HAL_StatusTypeDef LIN_send_break(UART_HandleTypeDef *huart);
HAL_StatusTypeDef LIN_send_bytes(
    UART_HandleTypeDef *huart,
    uint8_t *data,
    uint8_t length
);

/* Callback overrides */
void LIN_HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
void LIN_HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void LIN_HAL_UART_RxEventCallback(UART_HandleTypeDef * huart);

#endif /* LIN_DRIVER_H */