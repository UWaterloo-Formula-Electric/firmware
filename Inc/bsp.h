#ifndef __BSP_H
#define __BSP_H

#include "stm32f7xx_hal.h"

#define DEBUG_UART_HANDLE huart2
#define CAN_HANDLE hcan3

// Comment out to remove debug printing
#define DEBUG_ON

// Comment out to remove error printing
#define ERROR_PRINT_ON

#endif /* __BSP_H */
