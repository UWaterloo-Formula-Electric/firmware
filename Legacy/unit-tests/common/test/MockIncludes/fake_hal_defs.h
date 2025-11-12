#ifndef FAKE_HAL_DEFS_H
#define FAKE_HAL_DEFS_H
#include "portmacro.h"
#ifdef BOARD_TYPE_F7
#include "stm32f7xx_hal.h"
#else
#ifdef BOARD_TYPE_F0
#include "stm32f0xx_hal.h"
#endif
#endif

typedef enum
{
	HAL_OK       = 0x00U,
	HAL_ERROR    = 0x01U,
	HAL_BUSY     = 0x02U,
	HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

#define pdFALSE   ( ( BaseType_t ) 0 )
#define pdTRUE   ( ( BaseType_t ) 1 )
#define pdPASS   ( pdTRUE ) 
#define pdFAIL   ( pdFALSE )

#endif
