#ifndef __BSP_H
#define __BSP_H

#include "boardTypes.h"
#include "main.h"
#include "can.h"
#include "tim.h"
#include "usart.h"
#include "stdbool.h"
#include "iwdg.h"

//definitions for MISRA-C rules compliance
#ifndef IS_BOARD_F0
#define IS_BOARD_F0
#endif

#ifndef IS_BOARD_NUCLEO_F0
#define IS_BOARD_NUCLEO_F0 0
#endif

#if IS_BOARD_F0
#include "stm32f0xx_hal.h"

// TODO: Set these to the right values
#define DEBUG_UART_HANDLE huart2
#define CAN_HANDLE hcan
#define IWDG_HANDLE hiwdg
#define STATS_TIM_HANDLE htim2

//#define ERROR_LED_PIN LED_R_Pin
//#define ERROR_LED_PORT LED_R_GPIO_Port
//#define DEBUG_LED_PIN LD2_Pin
//#define DEBUG_LED_PORT LD2_GPIO_Port

#define EM_TOGGLE_BUTTON_PIN  BTN_EV_READ_Pin
#define EM_TOGGLE_BUTTON_PORT BTN_EV_READ_GPIO_Port
#define HV_TOGGLE_BUTTON_PIN  BTN_HV_READ_Pin
#define HV_TOGGLE_BUTTON_PORT BTN_HV_READ_GPIO_Port
#define TC_TOGGLE_BUTTON_PIN  BTN_TC_READ_Pin
#define TC_TOGGLE_BUTTON_PORT BTN_TC_READ_GPIO_Port
#define TV_TOGGLE_BUTTON_PIN  BTN_TV_READ_Pin
#define TV_TOGGLE_BUTTON_PORT BTN_TV_READ_GPIO_Port

#define EM_LED_Pin       EV_LED_EN_Pin
#define EM_LED_GPIO_Port EV_LED_EN_GPIO_Port
#define HV_LED_Pin       HV_LED_EN_Pin
#define HV_LED_GPIO_Port HV_LED_EN_GPIO_Port
#define TC_LED_Pin       TC_LED_EN_Pin
#define TC_LED_GPIO_Port TC_LED_EN_GPIO_Port
#define TV_LED_Pin       TV_LED_EN_Pin
#define TV_LED_GPIO_Port TV_LED_EN_GPIO_Port

#define MOT_RED_LED_Pin         MOT_LED_RED_EN_Pin
#define MOT_RED_LED_GPIO_Port   MOT_LED_RED_EN_GPIO_Port

#define HV_LED_ON HAL_GPIO_WritePin(HV_LED_GPIO_Port, HV_LED_Pin, GPIO_PIN_SET);
#define HV_LED_OFF HAL_GPIO_WritePin(HV_LED_GPIO_Port, HV_LED_Pin, GPIO_PIN_RESET);
#define EM_LED_ON HAL_GPIO_WritePin(EM_LED_GPIO_Port, EM_LED_Pin, GPIO_PIN_SET);
#define EM_LED_OFF HAL_GPIO_WritePin(EM_LED_GPIO_Port, EM_LED_Pin, GPIO_PIN_RESET);
#define TC_LED_ON HAL_GPIO_WritePin(TC_LED_GPIO_Port, TC_LED_Pin, GPIO_PIN_SET);
#define TC_LED_OFF HAL_GPIO_WritePin(TC_LED_GPIO_Port, TC_LED_Pin, GPIO_PIN_RESET);
#define TV_LED_ON HAL_GPIO_WritePin(TV_LED_GPIO_Port, TV_LED_Pin, GPIO_PIN_SET);
#define TV_LED_OFF HAL_GPIO_WritePin(TV_LED_GPIO_Port, TV_LED_Pin, GPIO_PIN_RESET);

#define IMD_FAIL_LED_ON HAL_GPIO_WritePin(IMD_LED_EN_GPIO_Port, IMD_LED_EN_Pin, GPIO_PIN_SET);
#define IMD_FAIL_LED_OFF HAL_GPIO_WritePin(IMD_LED_EN_GPIO_Port, IMD_LED_EN_Pin, GPIO_PIN_RESET);

#define AMS_FAIL_LED_ON HAL_GPIO_WritePin(AMS_LED_RED_EN_GPIO_Port, AMS_LED_RED_EN_Pin, GPIO_PIN_SET);
#define AMS_FAIL_LED_OFF HAL_GPIO_WritePin(AMS_LED_RED_EN_GPIO_Port, AMS_LED_RED_EN_Pin, GPIO_PIN_RESET);

#define ERROR_LED_PIN LED_R_Pin
#define ERROR_LED_PORT LED_R_GPIO_Port

#define BUZZER_ON HAL_GPIO_WritePin(BUZZER_EN_GPIO_Port, BUZZER_EN_Pin, GPIO_PIN_SET);
#define BUZZER_OFF HAL_GPIO_WritePin(BUZZER_EN_GPIO_Port, BUZZER_EN_Pin, GPIO_PIN_RESET);

#elif IS_BOARD_NUCLEO_F0
#include "stm32f0xx_hal.h"

#define DEBUG_UART_HANDLE huart2
#define IWDG_HANDLE hiwdg
#define CAN_HANDLE hcan
#define STATS_TIM_HANDLE htim2
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
#define ERROR_LED_PIN LD2_Pin
#define ERROR_LED_PORT LD2_GPIO_Port

#define HV_LED_ON
#define HV_LED_OFF
#define EM_LED_ON
#define EM_LED_OFF

#define IMD_FAIL_LED_ON
#define AMS_FAIL_LED_ON

#define BUZZER_ON
#define BUZZER_OFF

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

#define CONSOLE_PRINT_ON
#endif /* __BSP_H */
