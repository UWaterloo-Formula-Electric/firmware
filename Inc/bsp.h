#ifndef __BSP_H
#define __BSP_H

#include "boardTypes.h"
#include "main.h"
#include "can.h"

#if IS_BOARD_F7
#include "stm32f7xx_hal.h"

#define DEBUG_UART_HANDLE huart4 // TODO: Check this
#define CAN_HANDLE hcan3
#define DEBUG_LED_PIN LED_B_Pin
#define DEBUG_LED_PORT LED_B_GPIO_Port
#define ERROR_LED_PIN LED_R_Pin
#define ERROR_LED_PORT LED_R_GPIO_Port

#elif IS_BOARD_NUCLEO_F7
#include "stm32f7xx_hal.h"

#define DEBUG_UART_HANDLE huart3
#define CAN_HANDLE hcan1
#define DEBUG_LED_PIN LD2_Pin
#define DEBUG_LED_PORT LD2_GPIO_Port
#define ERROR_LED_PIN LD3_Pin
#define ERROR_LED_PORT LD3_GPIO_Port

#else

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message "BOARD_TYPE_NUCLEO_F7: " #BOARD_TYPE_NUCLEO_F7
#error Compiling for unkown board type

#endif

// Comment out to remove debug printing
#define DEBUG_ON

// Comment out to remove error printing
#define ERROR_PRINT_ON

#endif /* __BSP_H */
