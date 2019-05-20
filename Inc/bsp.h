#ifndef __BSP_H
#define __BSP_H

#include "boardTypes.h"
#include "main.h"
#include "can.h"
#include "tim.h"
#include "stdbool.h"
#include "iwdg.h"

#if IS_BOARD_F7
#include "stm32f7xx_hal.h"
#include "spi.h"
#include "adc.h"

#define CONT_POS_CLOSE	  HAL_GPIO_WritePin(CONT_POS_GPIO_Port,CONT_POS_Pin,GPIO_PIN_SET)
#define CONT_POS_OPEN	  HAL_GPIO_WritePin(CONT_POS_GPIO_Port,CONT_POS_Pin,GPIO_PIN_RESET)
#define CONT_NEG_CLOSE	  HAL_GPIO_WritePin(CONT_NEG_GPIO_Port,CONT_NEG_Pin,GPIO_PIN_SET)
#define CONT_NEG_OPEN	  HAL_GPIO_WritePin(CONT_NEG_GPIO_Port,CONT_NEG_Pin,GPIO_PIN_RESET)
#define PCDC_PC			  HAL_GPIO_WritePin(CONT_PRE_GPIO_Port,CONT_PRE_Pin,GPIO_PIN_SET)
#define PCDC_DC			  HAL_GPIO_WritePin(CONT_PRE_GPIO_Port,CONT_PRE_Pin,GPIO_PIN_RESET)
#define CONT_CHARGE_CLOSE HAL_GPIO_WritePin(CONT_PRE_GPIO_Port,CONT_PRE_Pin,GPIO_PIN_SET)
#define CONT_CHARGE_OPEN  HAL_GPIO_WritePin(CONT_PRE_GPIO_Port,CONT_PRE_Pin,GPIO_PIN_RESET)

#if BOARD_VERSION == 1

#define DEBUG_UART_HANDLE huart2
#define CAN_HANDLE hcan3
#define STATS_TIM_HANDLE htim4
#define ISO_SPI_HANDLE hspi4
#define HV_ADC_SPI_HANDLE hspi1
#define IMD_TIM_HANDLE htim3
#define IMD_TIM_INSTANCE TIM3
#define BRAKE_ADC_HANDLE hadc2
#define BRAKE_TIM_ADC_HANDLE htim6
#define DEBUG_LED_PIN LED_B_Pin
#define DEBUG_LED_PORT LED_B_GPIO_Port
#define ERROR_LED_PIN LED_Y_Pin
#define ERROR_LED_PORT LED_Y_GPIO_Port
#define IWDG_HANDLE hiwdg
#define DELAY_TIMER htim9
#define DELAY_TIMER_INSTANCE TIM9
// This doesn't exist on V1, but we need it so the code compiles still
#define CHARGER_CAN_HANDLE hcan3

#elif BOARD_VERSION == 2

#define DEBUG_UART_HANDLE huart2
#define CAN_HANDLE hcan3
#define CHARGER_CAN_HANDLE hcan1
#define STATS_TIM_HANDLE htim4
#define ISO_SPI_HANDLE hspi4
#define HV_ADC_SPI_HANDLE hspi1
#define IMD_TIM_HANDLE htim3
#define IMD_TIM_INSTANCE TIM3
#define BRAKE_ADC_HANDLE hadc2
#define BRAKE_TIM_ADC_HANDLE htim6
#define DEBUG_LED_PIN LED_B_Pin
#define DEBUG_LED_PORT LED_B_GPIO_Port
#define ERROR_LED_PIN LED_R_Pin
#define ERROR_LED_PORT LED_R_GPIO_Port
#define IWDG_HANDLE hiwdg
#define DELAY_TIMER htim9
#define DELAY_TIMER_INSTANCE TIM9

#else
#error "Unsupported board version"
#endif

#elif IS_BOARD_NUCLEO_F7
#include "stm32f7xx_hal.h"
#include "spi.h"

#define STATS_TIM_HANDLE htim3
#define DEBUG_UART_HANDLE huart3
#define CAN_HANDLE hcan1
#define CHARGER_CAN_HANDLE hcan1
#define DEBUG_LED_PIN LD2_Pin
#define DEBUG_LED_PORT LD2_GPIO_Port
#define ERROR_LED_PIN LD3_Pin
#define ERROR_LED_PORT LD3_GPIO_Port
#define IWDG_HANDLE hiwdg
#define ISO_SPI_HANDLE hspi4

#define CONT_POS_CLOSE	  asm("NOP")
#define CONT_POS_OPEN	  asm("NOP")
#define CONT_NEG_CLOSE	  asm("NOP")
#define CONT_NEG_OPEN	  asm("NOP")
#define PCDC_PC			  asm("NOP")
#define PCDC_DC			  asm("NOP")
#define CONT_CHARGE_CLOSE asm("NOP")
#define CONT_CHARGE_OPEN  asm("NOP")
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

#define CONSOLE_PRINT_ON

#endif /* __BSP_H */
