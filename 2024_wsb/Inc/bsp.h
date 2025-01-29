#ifndef __BSP_H
#define __BSP_H

// TODO: Adjust this for the WSB. This was copied

#include "adc.h"
#include "boardTypes.h"
#include "can.h"
#include "iwdg.h"
#include "main.h"
#include "stdbool.h"
#include "tim.h"
#include "usart.h"

// #define BOARD_DISABLE_CAN

#if IS_BOARD_F4
#include "stm32f4xx_hal.h"

// TODO: Set these to the right values (double check all these values)
#define DEBUG_UART_HANDLE huart1
#define CAN_HANDLE hcan1
#define MULTISENSOR_ADC_HANDLE hadc1
#define STATS_TIM_HANDLE htim12
#define IWDG_HANDLE hiwdg
#define ENCODER_TIM_HANDLE htim1
#define ENCODER_RPS_TIM_HANDLE htim13
#define ENCODER_RPS_TIM_MS 10  // edit TIM13 prescaler and arr to change this. currently set to 10ms

/* July 3, 2024 - Jacky
 * The 2024 WSBs no longer have these RED and YELLOW LEDs. generalErrorHandler.c requires these
 * ports and pins to be defined but I added an exception in that file just for the WSBs.
 * We can remove these ports and pins below (TODO: remove these when cleaning up)
 */
// #define ERROR_LED_PIN LED_R_Pin
// #define ERROR_LED_PORT LED_R_GPIO_Port
// #define WARNING_LED_PIN LED_Y_Pin
// #define WARNING_LED_PORT LED_Y_GPIO_Port
// #define DEBUG_LED_PIN LED_B_Pin
// #define DEBUG_LED_PORT LED_B_GPIO_Port

#elif IS_BOARD_NUCLEO_F4
#include "stm32f4xx_hal.h"

// #define DEBUG_UART_HANDLE huart2
// #define IWDG_HANDLE hiwdg
// #define CAN_HANDLE hcan1
// #define STATS_TIM_HANDLE htim2

#else

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message "BOARD_TYPE_NUCLEO_F4: " #BOARD_TYPE_NUCLEO_F4
#error Compiling for unkown board type

#endif

// Comment out to remove debug printing
#define DEBUG_ON

// Comment out to remove error printing
#define ERROR_PRINT_ON

#define CONSOLE_PRINT_ON
#endif /* __BSP_H */
