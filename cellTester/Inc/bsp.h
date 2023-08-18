#ifndef __BSP_H
#define __BSP_H

#include "boardTypes.h"
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "stdbool.h"
#include "iwdg.h"
#include "spi.h"

// cppcheck-suppress misra-c2012-20.9
#if IS_BOARD_F0
#include "stm32f0xx_hal.h"

// TODO: Set these to the right values
#define DEBUG_UART_HANDLE huart2
#define IWDG_HANDLE hiwdg
#define ISO_ADC_SPI_HANDLE hspi1

#else

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message "BOARD_TYPE_NUCLEO_F0: " #BOARD_TYPE_NUCLEO_F0
#error Compiling for unkown board type

#endif

#define DISABLE_CAN_FEATURES

// Comment out to remove debug printing
#define DEBUG_ON

// Comment out to remove error printing
#define ERROR_PRINT_ON

#define CONSOLE_PRINT_ON
#endif /* __BSP_H */
