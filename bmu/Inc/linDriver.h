#ifndef LIN_DRIVER_H
#define LIN_DRIVER_H

#include "stm32f7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

extern UART_HandleTypeDef huart5;
extern TIM_HandleTypeDef htim2;

/* LIN 2.x protocol constants */
#define LIN_BITS_PER_BYTE 10U
#define LIN_SYNC_BYTE 0x55U
#define LIN_MAX_DATA_SIZE 8U
#define LIN_MAX_RESPONSE_SIZE 9U
#define LIN_HEADER_SIZE 2U
#define LIN_MAX_FRAME_ID 0x3DU

/* LIN driver constants */
#define LIN_UART_HANDLE &huart5
#define LIN_TIM_HANDLE &htim2
#define LIN_DMA_BUFFER_SIZE 64U

typedef struct {
    uint8_t id;
    uint8_t pid;
    uint8_t buffer[LIN_MAX_RESPONSE_SIZE + LIN_HEADER_SIZE];
    uint8_t length;
    uint32_t duration_us;
    bool isRxBuffer;
    bool rxValid;
} LIN_Frame_t;

typedef struct {
    LIN_Frame_t activeFrame;
    uint32_t startTick;
    uint32_t endTick;
    uint8_t dmaBuffer[LIN_DMA_BUFFER_SIZE];
    uint8_t dmaIndex;
} LIN_Driver_t;

HAL_StatusTypeDef buildFrame(
    LIN_Frame_t *frame,
    uint8_t *data,
    uint8_t length);
HAL_StatusTypeDef setProtectedID(LIN_Frame_t *frame);
HAL_StatusTypeDef setChecksum(LIN_Frame_t *frame);
HAL_StatusTypeDef setUpNextFrame(LIN_Frame_t *frame);
HAL_StatusTypeDef txBreak(void);
HAL_StatusTypeDef trxFrame(void);

#endif /* LIN_DRIVER_H */