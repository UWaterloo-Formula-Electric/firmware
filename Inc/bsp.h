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
//#define ERROR_LED_PIN LED_R_Pin
//#define ERROR_LED_PORT LED_R_GPIO_Port
//#define DEBUG_LED_PIN LD2_Pin
//#define DEBUG_LED_PORT LD2_GPIO_Port
#define EM_TOGGLE_BUTTON_PIN EM_ENABLE_Pin
#define EM_TOGGLE_BUTTON_PORT EM_ENABLE_GPIO_Port
#define HV_TOGGLE_BUTTON_PIN HV_ENABLE_Pin
#define HV_TOGGLE_BUTTON_PORT HV_ENABLE_GPIO_Port

#elif IS_BOARD_NUCLEO_F0
#include "stm32f0xx_hal.h"

#define DEBUG_UART_HANDLE huart2
#define CAN_HANDLE hcan
//#define DEBUG_LED_PIN LD2_Pin
//#define DEBUG_LED_PORT LD2_GPIO_Port
//#define ERROR_LED_PIN LD2_Pin
//#define ERROR_LED_PORT LD2_GPIO_Port
#define EM_TOGGLE_BUTTON_PIN B1_Pin
#define EM_TOGGLE_BUTTON_PORT B1_GPIO_Port
#define HV_TOGGLE_BUTTON_PIN B2_Pin
#define HV_TOGGLE_BUTTON_PORT B2_GPIO_Port
#define EM_LED_Pin LD2_Pin
#define EM_LED_GPIO_Port LD2_GPIO_Port
#define HV_LED_Pin LD2_Pin
#define HV_LED_GPIO_Port LD2_GPIO_Port

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

#endif /* __BSP_H */
