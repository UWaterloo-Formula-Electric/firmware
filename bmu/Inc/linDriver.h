#ifndef LIN_DRIVER_H
#define LIN_DRIVER_H

#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "stm32f7xx_hal.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

extern UART_HandleTypeDef huart5;
extern TIM_HandleTypeDef htim2; // free-running, 32-bit, 1MHz

/* LIN 2.x protocol constants */
#define LIN_SYNC_BYTE         0x55U
#define LIN_MAX_DATA_SIZE     8U
#define LIN_MAX_RESPONSE_SIZE 9U // data + checksum
#define LIN_HEADER_SIZE       2U // sync + PID
#define LIN_MAX_FRAME_ID      0x3DU

/* LIN driver constants */
#define LIN_UART_HANDLE       (&huart5)
#define LIN_TIM_HANDLE        (&htim2) 
#define LIN_RX_BUFFER_SIZE    32U
#define LIN_BREAK_TIMEOUT_US  2000U

typedef struct {
    uint8_t id;
    uint8_t pid;
    uint8_t buffer[LIN_MAX_RESPONSE_SIZE + LIN_HEADER_SIZE];
    uint8_t length; // total bytes (sync + PID + data + checksum)
    uint32_t duration_us;
    bool isRxBuffer;
} LIN_Frame_t;

// Set frame->id and frame->isRxBuffer first
HAL_StatusTypeDef buildFrame(
    LIN_Frame_t *frame,
    const uint8_t *data,
    uint8_t dataLength);
HAL_StatusTypeDef transferFrame(LIN_Frame_t *frame);
HAL_StatusTypeDef linPublish(
    uint8_t id,
    const uint8_t *data,
    uint8_t dataLength);
HAL_StatusTypeDef linRequest(
    uint8_t id,
    uint8_t *out,
    uint8_t dataLength);

HAL_StatusTypeDef registerLinCommands(void);

#endif