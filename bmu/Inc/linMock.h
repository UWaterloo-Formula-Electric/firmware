#ifndef LIN_MOCK_H
#define LIN_MOCK_H

#include "linDriver.h"
#include "stm32f7xx_hal.h"
#include <stdint.h>

typedef struct {
    uint8_t id;
    uint8_t pid;
    uint8_t bytes[LIN_MAX_RESPONSE_SIZE];
    uint8_t length;
} LIN_MockResponse_t;

void buildMockResponse(
    uint8_t id,
    uint8_t *data,
    uint8_t dataLength);
HAL_StatusTypeDef mockLinPublish(
    uint8_t id,
    uint8_t *data,
    uint8_t dataLength
);
HAL_StatusTypeDef mockLinRequest(
    uint8_t id,
    uint8_t *data,
    uint8_t dataLength);
HAL_StatusTypeDef registerLinCommands(void);

#endif /* Include Guard: LIN_MOCK_H */