#ifndef SD_LOGGING_H
#define SD_LOGGING_H

#include <stdint.h>
#include "debug.h"

HAL_StatusTypeDef initSdLoggingQueue(void);

extern QueueHandle_t sdLoggingQueue;

typedef struct can_msg_t{
    uint32_t id;
    uint8_t data[8];
} can_msg_t;

#endif //SD_LOGGING_H
