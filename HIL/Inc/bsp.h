/**
  *****************************************************************************
  * @file    bsp.h
  * @brief   Board support package (BSP) header file for the HIL bench-test
  *          board.
  * @details Instead of directly referring to hardware in source code, create
  * a define here and use that everywhere else, the same convention every
  * other board uses.
  *
  * HIL never ships in the car and is not an addressed node on the vehicle
  * CAN bus - see ../README.md. TODO: confirm/replace the handles and pins
  * below once the schematic exists and Cube-F7-Src-respin/ has been
  * generated for STM32F769BIT6.
  ******************************************************************************
  */

#ifndef HIL_BSP_H
#define HIL_BSP_H

#include "boardTypes.h"
#include "main.h"
#include "can.h"
#include "usart.h"
#include "iwdg.h"
#include "stdbool.h"

#if IS_BOARD_F7
#include "stm32f7xx_hal.h"

// TODO: confirm these against the real schematic
#define DEBUG_UART_HANDLE huart2
#define CAN_HANDLE hcan1
#define IWDG_HANDLE hiwdg
#define DEBUG_LED_PIN LED_B_Pin
#define DEBUG_LED_PORT LED_B_GPIO_Port
#define ERROR_LED_PIN LED_R_Pin
#define ERROR_LED_PORT LED_R_GPIO_Port

#else

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message "BOARD_TYPE_F7: " #BOARD_TYPE_F7
#error Compiling for unknown board type

#endif

// Comment out to remove debug printing
#define DEBUG_ON

// Comment out to remove error printing
#define ERROR_PRINT_ON

#define CONSOLE_PRINT_ON

#endif /* HIL_BSP_H */
