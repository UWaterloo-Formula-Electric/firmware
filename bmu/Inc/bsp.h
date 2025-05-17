/**
  *****************************************************************************
  * @file    bsp.h
  * @author  Richard Matthews
  * @brief   Board support package (BSP) header file.
  * @details This header file is a simple BSP, specifying which hardware to use
  * on different build targets. Instead of directly refering to hardware in
  * source code, create a define here and use that, then ensure the define
  * exists for each build target it applies to.
  *
  ******************************************************************************
  */

#ifndef __BSP_H
#define __BSP_H

#include "boardTypes.h"
#include "main.h"
#include "can.h"
#include "tim.h"
#include "stdbool.h"
#include "iwdg.h"
#include "usart.h"

#if IS_BOARD_F7
#include "stm32f7xx_hal.h"
#include "spi.h"
#include "adc.h"

#define CONT_POS_CLOSE	    HAL_GPIO_WritePin(CONT_POS_GPIO_Port,CONT_POS_Pin,GPIO_PIN_SET)
#define CONT_POS_OPEN	    HAL_GPIO_WritePin(CONT_POS_GPIO_Port,CONT_POS_Pin,GPIO_PIN_RESET)
#define CONT_NEG_CLOSE	    HAL_GPIO_WritePin(CONT_NEG_GPIO_Port,CONT_NEG_Pin,GPIO_PIN_SET)
#define CONT_NEG_OPEN	    HAL_GPIO_WritePin(CONT_NEG_GPIO_Port,CONT_NEG_Pin,GPIO_PIN_RESET)
#define PCDC_PC			    HAL_GPIO_WritePin(CONT_PRE_GPIO_Port,CONT_PRE_Pin,GPIO_PIN_SET)
#define PCDC_DC			    HAL_GPIO_WritePin(CONT_PRE_GPIO_Port,CONT_PRE_Pin,GPIO_PIN_RESET)
#define CONT_CHARGE_CLOSE   HAL_GPIO_WritePin(CONT_PRE_GPIO_Port,CONT_PRE_Pin,GPIO_PIN_SET)
#define CONT_CHARGE_OPEN    HAL_GPIO_WritePin(CONT_PRE_GPIO_Port,CONT_PRE_Pin,GPIO_PIN_RESET)
#define AMS_CONT_CLOSE      HAL_GPIO_WritePin(AMS_CONT_GPIO_Port,AMS_CONT_Pin,GPIO_PIN_SET)
#define AMS_CONT_OPEN       HAL_GPIO_WritePin(AMS_CONT_GPIO_Port,AMS_CONT_Pin,GPIO_PIN_RESET)
#define DC_DC_ON            HAL_GPIO_WritePin(CONT_DC_DC_GPIO_Port,CONT_DC_DC_Pin,GPIO_PIN_SET);
#define DC_DC_OFF           HAL_GPIO_WritePin(CONT_DC_DC_GPIO_Port,CONT_DC_DC_Pin,GPIO_PIN_RESET);
#define TSSI_GREEN_ON       HAL_GPIO_WritePin(TSSI_GREEN_EN_GPIO_Port, TSSI_GREEN_EN_Pin, GPIO_PIN_SET)
#define TSSI_GREEN_OFF      HAL_GPIO_WritePin(TSSI_GREEN_EN_GPIO_Port, TSSI_GREEN_EN_Pin, GPIO_PIN_RESET)
#define TSSI_RED_ON         HAL_TIM_Base_Start_IT(&TSSI_TIMER_HANDLE);
#define TSSI_RED_OFF        HAL_TIM_Base_Stop_IT(&TSSI_TIMER_HANDLE);
#define DEBUG_UART_HANDLE huart2
#define CAN_HANDLE hcan3
#define CHARGER_CAN_HANDLE hcan1
#define STATS_TIM_HANDLE htim4
#define ISO_SPI_HANDLE hspi4
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
#define FAN_HANDLE htim12
#define HW_CHECK_HANDLE htim2
#define CONT_SENSE_ADC_HANDLE hadc1
#define CONT_SENSE_TIM htim7
#define TSSI_TIMER_HANDLE htim8

typedef enum taskId_e{
    FSM_TASK_ID = 1,                // 1
    BATTERY_TASK_ID,                // 2
    BSPD_SENSE_TASK_ID,             // 3
    HV_MEASURE_TASK_ID,             // 4
    IMD_TASK_ID,                    // 5
    FAULT_TASK_ID,                  // 6
    SOC_TASK_ID,                    // 7
    CAN_CELL_SEND_TASK_ID,          // 8
    CONT_CURRENT_SENSE_TASK_ID,     // 9
    IVT_TASK_ID,                     // 10
}taskId_e;

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
#define FAN_HANDLE htim10

#define CONT_POS_CLOSE	  asm("NOP")
#define CONT_POS_OPEN	  asm("NOP")
#define CONT_NEG_CLOSE	  asm("NOP")
#define CONT_NEG_OPEN	  asm("NOP")
#define PCDC_PC	          asm("NOP")
#define PCDC_DC           asm("NOP")
#define CONT_CHARGE_CLOSE asm("NOP")
#define CONT_CHARGE_OPEN  asm("NOP")
#define DC_DC_ON          asm("NOP")
#define DC_DC_OFF         asm("NOP")

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
