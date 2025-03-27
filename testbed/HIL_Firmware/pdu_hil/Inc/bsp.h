#ifndef __BSP_H
#define __BSP_H

#include "boardTypes.h"
#include "main.h"
#include "can.h"
#include "tim.h"
#include <stdbool.h>
#include "i2c.h"
#include "dac.h"
#include "can.h"

#if IS_BOARD_F7
#include "stm32f7xx_hal.h"

#define DEBUG_UART_HANDLE huart4
#define CAN_HANDLE hcan1

#else
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message "BOARD_TYPE_NUCLEO_F0: " #BOARD_TYPE_NUCLEO_F0
#error Compiling for unkown board type


#endif

#define DEBUG_ON

// Comment out to remove error printing
#define ERROR_PRINT_ON

#define CONSOLE_PRINT_ON
#endif /* __BSP_H */

