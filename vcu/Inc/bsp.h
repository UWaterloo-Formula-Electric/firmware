#ifndef __BSP_H
#define __BSP_H

#include "boardTypes.h"
#include "main.h"
#include "can.h"
#include "tim.h"
#include "iwdg.h"
#include <stdbool.h>
#include "usart.h"

#if IS_BOARD_F7
#include "stm32f7xx_hal.h"
#include "adc.h"

#define STATS_TIM_HANDLE htim3
#define BRAKE_ADC_TIM_HANDLE htim6
#define DEBUG_UART_HANDLE huart2
#define CAN_HANDLE hcan3
#define ADC_HANDLE hadc1
#define IWDG_HANDLE hiwdg
#define DEBUG_LED_PIN LED_B_Pin
#define DEBUG_LED_PORT LED_B_GPIO_Port
#define ERROR_LED_PIN LED_Y_Pin
#define ERROR_LED_PORT LED_Y_GPIO_Port


#define PP_5V0_ENABLE 	HAL_GPIO_WritePin(PP_5V0_EN_GPIO_Port,PP_5V0_EN_Pin,GPIO_PIN_SET)
#define PP_5V0_DISABLE	HAL_GPIO_WritePin(PP_5V0_EN_GPIO_Port,PP_5V0_EN_Pin,GPIO_PIN_RESET)
#define PP_BB_ENABLE 	HAL_GPIO_WritePin(PP_BB_EN_GPIO_Port,PP_BB_EN_Pin,GPIO_PIN_SET)
#define PP_BB_DISABLE	HAL_GPIO_WritePin(PP_BB_EN_GPIO_Port,PP_BB_EN_Pin,GPIO_PIN_RESET)


#elif IS_BOARD_NUCLEO_F7
#include "stm32f7xx_hal.h"


#define STATS_TIM_HANDLE htim3
#define DEBUG_UART_HANDLE huart3
#define ADC_HANDLE hadc1
#define CAN_HANDLE hcan1
#define IWDG_HANDLE hiwdg
#define DEBUG_LED_PIN LD2_Pin
#define DEBUG_LED_PORT LD2_GPIO_Port
#define ERROR_LED_PIN LD3_Pin
#define ERROR_LED_PORT LD3_GPIO_Port



#else

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message "BOARD_TYPE_NUCLEO_F7: " #BOARD_TYPE_NUCLEO_F7
#error Compiling for unknown board type

#endif

// Comment out to remove debug printing
#define DEBUG_ON

// Comment out to remove error printing
#define ERROR_PRINT_ON

#define CONSOLE_PRINT_ON

#endif /* __BSP_H */
