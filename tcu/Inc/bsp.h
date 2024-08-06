#ifndef __BSP_H
#define __BSP_H

#include "boardTypes.h"
#include "main.h"
#include "can.h"
#include "iwdg.h"
#include <stdbool.h>
#include "usart.h"

#if IS_BOARD_F4
#include "stm32f4xx_hal.h"

#define CAN_HANDLE hcan1
#define DEBUG_UART_HANDLE huart1
#define IWDG_HANDLE hiwdg

#else

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message "BOARD_TYPE_NUCLEO_F4: " #BOARD_TYPE_NUCLEO_F4
#error Compiling for unknown board type

#endif

// Comment out to remove debug printing
#define DEBUG_ON

// Comment out to remove error printing
#define ERROR_PRINT_ON

#define CONSOLE_PRINT_ON

#endif /* __BSP_H */
