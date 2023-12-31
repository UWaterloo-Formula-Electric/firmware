#ifndef __BSP_H
#define __BSP_H

#include "boardTypes.h"
#include "main.h"
#include "can.h"

#if IS_BOARD_F0
#include "stm32f0xx_hal.h"

// TODO: Set these to the right values
#define DEBUG_UART_HANDLE huart2
#define CAN_HANDLE hcan

#elif IS_BOARD_NUCLEO_F0
#include "stm32f0xx_hal.h"

#define DEBUG_UART_HANDLE huart2
#define CAN_HANDLE hcan

#else

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message "BOARD_TYPE_NUCLEO_F0: " #BOARD_TYPE_NUCLEO_F0
#error Compiling for unkown board type

#endif

// Comment out to remove debug printing
#define DEBUG_ON

// Comment out to remove error printing
#define ERROR_PRINT_ON

// Comment out to remove CLI printing
#define CONSOLE_PRINT_ON

#endif /* __BSP_H */
