#ifndef __CANLOGGER_H__
#define __CANLOGGER_H__

#include <stdbool.h>

#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "stream_buffer.h"

typedef struct {
    uint32_t id;
    uint8_t data[8];
} CanMsg;

extern volatile bool isSDInserted;
extern StreamBufferHandle_t canLogSB;
extern volatile bool isCanLogEnabled;
HAL_StatusTypeDef canLogSB_init();
#endif  // __CANLOGGER_H__