#ifndef LIN_DRIVER_H
#define LIN_DRIVER_H

#include "stm32f7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* LIN 2.x protocol constants */
#define LIN_BITS_PER_BYTE 10U
#define LIN_SYNC_BYTE 0x55U
#define LIN_MAX_DATA_SIZE 8U
#define LIN_MAX_RESPONSE_SIZE 9U
#define LIN_HEADER_SIZE 2U
#define LIN_MAX_FRAME_ID 0x3DU
#define LIN_DIAG_MASTER_REQUEST_FRAME 0x3CU
#define LIN_DIAG_SLAVE_RESPONSE_FRAME 0x3DU

/* LIN driver constants */
#define LIN_MAX_FRAMES 16U
#define LIN_MAX_SLOTS 16U
#define LIN_DMA_BUFFER_SIZE 256U

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

typedef enum {
    LIN_READY,
    LIN_TX_BREAK,
    LIN_TX_SYNC,
    LIN_TX_PID,
    LIN_TRX_DATA,
    LIN_TRX_CHECKSUM,
    LIN_DONE,
    LIN_STOP,
    LIN_RESTART,
} LIN_State_t;

typedef enum {
    LIN_ERROR_NONE,
    LIN_TIMEOUT,
    LIN_BAD_PARAMS,
    LIN_OVERRUN_ERROR,
    LIN_FRAMING_ERROR,
    LIN_NOISE_ERROR,
    LIN_BIT_ERROR,
    LIN_CHECKSUM_ERROR,
    LIN_UART_ERROR,
    LIN_DMA_ERROR
} LIN_Error_t;

typedef struct {
    uint8_t id;
    uint8_t pid;
    LIN_Direction_t direction;
    LIN_FrameType_t frameType;
    LIN_ChecksumType_t checksumType;
    uint8_t headerBytes[LIN_HEADER_SIZE];
    uint8_t responseBytes[LIN_MAX_RESPONSE_SIZE];
    // Number of data bytes only
    uint8_t responseSize;
    uint8_t updateBuffer[LIN_MAX_DATA_SIZE];
    // Number of update data bytes
    uint8_t updateSize;
    bool rxValidResponse;
    bool txUpdatePending;
} LIN_Frame_t;

typedef struct {
    uint8_t frameIndex;
    uint8_t maxTries;
    uint32_t duration_us;
} LIN_Slot_t;

typedef struct {
    LIN_Frame_t frames[LIN_MAX_FRAMES];
    LIN_Slot_t slots[LIN_MAX_SLOTS];
    uint8_t frameCount;
    uint8_t slotCount;
    uint8_t slotIndex;
    uint32_t startTick;
    uint32_t endTick;
    uint32_t delay_us;
} LIN_Schedule_t;

typedef struct {
    LIN_State_t state;
    LIN_Error_t error;
    LIN_Slot_t *slot;
    LIN_Frame_t *frame;
    uint8_t tryCount;
    uint8_t responseByteCount;
} LIN_Transaction_t;

typedef struct {
    TIM_HandleTypeDef *htim;
    UART_HandleTypeDef *huart;
} LIN_Settings_t;

typedef struct {
    LIN_Settings_t settings;
    LIN_Schedule_t schedule;
    LIN_Transaction_t transaction;
    uint8_t dmaBuffer[LIN_DMA_BUFFER_SIZE];
    uint16_t dmaIndex;
    uint8_t echoBytes[LIN_HEADER_SIZE + LIN_MAX_RESPONSE_SIZE];
    uint8_t echoIndex;
    bool isRunning;
} LIN_Driver_t;

HAL_StatusTypeDef LIN_setSettings(LIN_Settings_t *settings);
HAL_StatusTypeDef LIN_setSchedule(LIN_Schedule_t *schedule);

HAL_StatusTypeDef LIN_setProtectedID(LIN_Frame_t *frame);
HAL_StatusTypeDef LIN_setChecksum(LIN_Frame_t *frame);
HAL_StatusTypeDef LIN_setChecksumType(LIN_Frame_t *frame);

HAL_StatusTypeDef LIN_deepcopyFrame(
    const LIN_Frame_t *original,
    LIN_Frame_t *copy
);
HAL_StatusTypeDef LIN_updateResponse(
    uint8_t *data,
    uint8_t size,
    uint8_t index
);

HAL_StatusTypeDef LIN_startSchedule(void);

void LIN_HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void LIN_HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
void LIN_HAL_UART_IRQHandler(UART_HandleTypeDef *huart);
void LIN_HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);

#endif /* LIN_DRIVER_H */