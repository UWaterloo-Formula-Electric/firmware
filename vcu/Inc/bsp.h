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
#define ADC_TIM_HANDLE htim6
#define DEBUG_UART_HANDLE huart4
#define CAN_HANDLE hcan3
#define ADC_HANDLE hadc1
#define IWDG_HANDLE hiwdg
#define DEBUG_LED_PIN LED_B_Pin
#define DEBUG_LED_PORT LED_B_GPIO_Port
#define ERROR_LED_PIN LED_R_Pin
#define ERROR_LED_PORT LED_R_GPIO_Port

// Read pins (we have 4 buttons)
#define EM_TOGGLE_BUTTON_PIN            BTN_EM_SNS_Pin
#define EM_TOGGLE_BUTTON_PORT           BTN_EM_SNS_GPIO_Port
#define HV_TOGGLE_BUTTON_PIN            BTN_HV_SNS_Pin
#define HV_TOGGLE_BUTTON_PORT           BTN_HV_SNS_GPIO_Port
#define TC_TOGGLE_BUTTON_PIN            BTN_TC_SNS_Pin
#define TC_TOGGLE_BUTTON_PORT           BTN_TC_SNS_GPIO_Port
#define ENDURANCE_TOGGLE_BUTTON_PIN     BTN_ENDUR_SNS_Pin
#define ENDURANCE_TOGGLE_BUTTON_PORT    BTN_ENDUR_SNS_GPIO_Port

// Additionally, each button has LED
#define EM_LED_Pin                      LED_BTN_EM_EN_Pin
#define EM_LED_GPIO_Port                LED_BTN_EM_EN_GPIO_Port
#define HV_LED_Pin                      LED_BTN_HV_EN_Pin
#define HV_LED_GPIO_Port                LED_BTN_HV_EN_GPIO_Port
#define TC_LED_Pin                      LED_BTN_TC_EN_Pin
#define TC_LED_GPIO_Port                LED_BTN_TC_EN_GPIO_Port
#define ENDURANCE_LED_Pin               LED_BTN_ENDUR_EN_Pin
#define ENDURANCE_LED_GPIO_Port         LED_BTN_ENDUR_EN_GPIO_Port

// Fault LEDs
#define IMD_FAULT_LED_EN_Pin               IMD_FAULT_LED_EN_Pin
#define IMD_FAULT_LED_EN_GPIO_Port         IMD_FAULT_LED_EN_GPIO_Port
#define AMS_FAULT_LED_EN_Pin               AMS_FAULT_LED_EN_Pin
#define AMS_FAULT_LED_EN_GPIO_Port         AMS_FAULT_LED_EN_GPIO_Port
#define MC_FAULT_LED_EN_Pin                MC_FAULT_LED_EN_Pin
#define MC_FAULT_LED_EN_GPIO_Port          MC_FAULT_LED_EN_GPIO_Port
#define MOT_FAULT_LED_EN_Pin               MOT_FAULT_LED_EN_Pin
#define MOT_FAULT_LED_EN_GPIO_Port         MOT_FAULT_LED_EN_GPIO_Port

/* High-level macros for the users */
#define BUZZER_ENABLE   HAL_GPIO_WritePin(BUZZER_EN_GPIO_Port,BUZZER_EN_Pin,GPIO_PIN_SET)
#define BUZZER_DISABLE   HAL_GPIO_WritePin(BUZZER_EN_GPIO_Port,BUZZER_EN_Pin,GPIO_PIN_RESET)

// BUTTON LEDS
#define HV_LED_ON HAL_GPIO_WritePin(HV_LED_GPIO_Port, HV_LED_Pin, GPIO_PIN_SET);
#define HV_LED_OFF HAL_GPIO_WritePin(HV_LED_GPIO_Port, HV_LED_Pin, GPIO_PIN_RESET);

#define EM_LED_ON HAL_GPIO_WritePin(EM_LED_GPIO_Port, EM_LED_Pin, GPIO_PIN_SET);
#define EM_LED_OFF HAL_GPIO_WritePin(EM_LED_GPIO_Port, EM_LED_Pin, GPIO_PIN_RESET);

#define TC_LED_ON HAL_GPIO_WritePin(TC_LED_GPIO_Port, TC_LED_Pin, GPIO_PIN_SET);
#define TC_LED_OFF HAL_GPIO_WritePin(TC_LED_GPIO_Port, TC_LED_Pin, GPIO_PIN_RESET);

#define ENDURANCE_LED_ON HAL_GPIO_WritePin(ENDURANCE_LED_GPIO_Port, ENDURANCE_LED_Pin, GPIO_PIN_SET);
#define ENDURANCE_LED_OFF HAL_GPIO_WritePin(ENDURANCE_LED_GPIO_Port, ENDURANCE_LED_Pin, GPIO_PIN_RESET);

// FAULT LEDS
#define IMD_FAIL_LED_ON HAL_GPIO_WritePin(IMD_FAULT_LED_EN_GPIO_Port, IMD_FAULT_LED_EN_Pin, GPIO_PIN_SET);
#define IMD_FAIL_LED_OFF HAL_GPIO_WritePin(IMD_FAULT_LED_EN_GPIO_Port, IMD_FAULT_LED_EN_Pin, GPIO_PIN_RESET);

#define AMS_FAIL_LED_ON HAL_GPIO_WritePin(AMS_FAULT_LED_EN_GPIO_Port, AMS_FAULT_LED_EN_Pin, GPIO_PIN_SET);
#define AMS_FAIL_LED_OFF HAL_GPIO_WritePin(AMS_FAULT_LED_EN_GPIO_Port, AMS_FAULT_LED_EN_Pin, GPIO_PIN_RESET);

#define MC_FAIL_LED_ON HAL_GPIO_WritePin(MC_FAULT_LED_EN_GPIO_Port, MC_FAULT_LED_EN_Pin, GPIO_PIN_SET);
#define MC_FAIL_LED_OFF HAL_GPIO_WritePin(MC_FAULT_LED_EN_GPIO_Port, MC_FAULT_LED_EN_Pin, GPIO_PIN_RESET);

#define MOT_FAIL_LED_ON HAL_GPIO_WritePin(MOT_FAULT_LED_EN_GPIO_Port, MOT_FAULT_LED_EN_Pin, GPIO_PIN_SET);
#define MOT_FAIL_LED_OFF HAL_GPIO_WritePin(MOT_FAULT_LED_EN_GPIO_Port, MOT_FAULT_LED_EN_Pin, GPIO_PIN_RESET);

/* Below is for Nucleo!! */
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
