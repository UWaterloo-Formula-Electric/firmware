#ifndef __BSP_H
#define __BSP_H

#include "boardTypes.h"
#include "main.h"
#include "can.h"
#include "tim.h"
#include "adc.h"
#include "usart.h"
#include "iwdg.h"
#include "stdbool.h"

#if IS_BOARD_F7
#include "stm32f7xx_hal.h"

#define STATS_TIM_HANDLE htim3      // For FreeRTOS
#define ADC_TIM_HANDLE htim6        // For ADC1 (acts as trigger source)
#define DEBUG_UART_HANDLE huart4
#define CAN_HANDLE hcan3
#define ADC1_HANDLE hadc1
#define ADC3_HANDLE hadc3
#define I2C_HANDLE hi2c1
#define IWDG_HANDLE hiwdg

#define DEBUG_BLUE_LED_PIN LED_B_Pin
#define DEBUG_BLUE_LED_PORT LED_B_GPIO_Port
#define DEBUG_YELLOW_LED_PIN LED_Y_Pin
#define DEBUG_YELLOW_LED_PORT LED_Y_GPIO_Port
#define ERROR_LED_PIN LED_R_Pin
#define ERROR_LED_PORT LED_R_GPIO_Port

#define CAN_ENABLED

// TODO: create defines for new ones 
#define RADIATOR_EN HAL_GPIO_WritePin(RAD_EN_GPIO_Port, RAD_EN_Pin, GPIO_PIN_SET)
#define RADIATOR_DISABLE HAL_GPIO_WritePin(RAD_EN_GPIO_Port, RAD_EN_Pin, GPIO_PIN_RESET)
#define INVERTER_EN HAL_GPIO_WritePin(INV_EN_GPIO_Port, INV_EN_Pin, GPIO_PIN_SET)
#define INVERTER_DISABLE HAL_GPIO_WritePin(INV_EN_GPIO_Port, INV_EN_Pin, GPIO_PIN_RESET)
#define ACC_FANS_EN HAL_GPIO_WritePin(ACC_FANS_EN_GPIO_Port, ACC_FANS_EN_Pin, GPIO_PIN_SET)
#define ACC_FANS_DISABLE HAL_GPIO_WritePin(ACC_FANS_EN_GPIO_Port, ACC_FANS_EN_Pin, GPIO_PIN_RESET)
#define BRAKE_LIGHT_ENABLE HAL_GPIO_WritePin(BRAKE_LIGHT_EN_GPIO_Port, BRAKE_LIGHT_EN_Pin, GPIO_PIN_SET)
#define BRAKE_LIGHT_DISABLE HAL_GPIO_WritePin(BRAKE_LIGHT_EN_GPIO_Port, BRAKE_LIGHT_EN_Pin, GPIO_PIN_RESET)
#define TCU_EN HAL_GPIO_WritePin(TCU_EN_GPIO_Port, TCU_EN_Pin, GPIO_PIN_SET)
#define TCU_DISABLE HAL_GPIO_WritePin(TCU_EN_GPIO_Port, TCU_EN_Pin, GPIO_PIN_RESET)
#define WSB_EN HAL_GPIO_WritePin(WSB_EN_GPIO_Port, WSB_EN_Pin, GPIO_PIN_SET)
#define WSB_DISABLE HAL_GPIO_WritePin(WSB_EN_GPIO_Port, WSB_EN_Pin, GPIO_PIN_RESET)
#define BMU_EN HAL_GPIO_WritePin(BMU_EN_GPIO_Port, BMU_EN_Pin, GPIO_PIN_SET)
#define BMU_DISABLE HAL_GPIO_WritePin(BMU_EN_GPIO_Port, BMU_EN_Pin, GPIO_PIN_RESET)
#define CDU_EN HAL_GPIO_WritePin(CDU_EN_GPIO_Port, CDU_EN_Pin, GPIO_PIN_SET)
#define CDU_DISABLE HAL_GPIO_WritePin(CDU_EN_GPIO_Port, CDU_EN_Pin, GPIO_PIN_RESET)
#define PUMP_2_EN HAL_GPIO_WritePin(PUMP_2_EN_GPIO_Port, PUMP_2_EN_Pin, GPIO_PIN_SET)
#define PUMP_2_DISABLE HAL_GPIO_WritePin(PUMP_2_EN_GPIO_Port, PUMP_2_EN_Pin, GPIO_PIN_RESET)
#define PUMP_1_EN HAL_GPIO_WritePin(PUMP_1_EN_GPIO_Port, PUMP_1_EN_Pin, GPIO_PIN_SET)
#define PUMP_1_DISABLE HAL_GPIO_WritePin(PUMP_1_EN_GPIO_Port, PUMP_1_EN_Pin, GPIO_PIN_RESET)

// AUX
#define AUX_1_EN HAL_GPIO_WritePin(AUX_1_PWR_GPIO_Port, AUX_1_PWR_Pin, GPIO_PIN_SET)
#define AUX_1_DISABLE HAL_GPIO_WritePin(AUX_1_PWR_GPIO_Port, AUX_1_PWR_Pin, GPIO_PIN_RESET)
#define AUX_2_EN HAL_GPIO_WritePin(AUX_2_PWR_GPIO_Port, AUX_2_PWR_Pin, GPIO_PIN_SET)
#define AUX_2_DISABLE HAL_GPIO_WritePin(AUX_2_PWR_GPIO_Port, AUX_2_PWR_Pin, GPIO_PIN_RESET)
#define AUX_3_EN HAL_GPIO_WritePin(AUX_3_PWR_GPIO_Port, AUX_3_PWR_Pin, GPIO_PIN_SET)
#define AUX_3_DISABLE HAL_GPIO_WritePin(AUX_3_PWR_GPIO_Port, AUX_3_PWR_Pin, GPIO_PIN_RESET)
#define AUX_4_EN HAL_GPIO_WritePin(AUX_4_PWR_GPIO_Port, AUX_4_PWR_Pin, GPIO_PIN_SET)
#define AUX_4_DISABLE HAL_GPIO_WritePin(AUX_4_PWR_GPIO_Port, AUX_4_PWR_Pin, GPIO_PIN_RESET)

// charge stat (2), temp alert, 
/* See Table 2 in BQ24650RVAT's datasheet for logic */
#define READ_CHARGE_STAT_1 (HAL_GPIO_ReadPin(CHG_STAT_1_GPIO_Port, CHG_STAT_1_Pin))
#define READ_CHARGE_STAT_2 (HAL_GPIO_ReadPin(CHG_STAT_2_GPIO_Port, CHG_STAT_2_Pin))
#define READ_TEMP_ALERT_PIN (HAL_GPIO_ReadPin(TEMP_ALERT_GPIO_Port, TEMP_ALERT_Pin))

/*********** For NUCLEO! ***********/
#elif IS_BOARD_NUCLEO_F7
#include "stm32f7xx_hal.h"

#define DEBUG_UART_HANDLE huart3
#define CAN_HANDLE hcan1
#define STATS_TIM_HANDLE htim3
#define ADC_HANDLE hadc3
#define IWDG_HANDLE hiwdg
#define DEBUG_LED_PIN LD2_Pin
#define DEBUG_LED_PORT LD2_GPIO_Port
#define ERROR_LED_PIN LD3_Pin
#define ERROR_LED_PORT LD3_GPIO_Port

//#define CAN_ENABLED
// asm"nop" means no operation (used as placeholder)
#define FAN_BATT_ENABLE		asm("NOP")
#define FAN_BATT_DISABLE 	asm("NOP")
#define FAN_LEFT_ENABLE		asm("NOP")
#define FAN_LEFT_DISABLE	asm("NOP")
#define FAN_RIGHT_ENABLE	asm("NOP")
#define FAN_RIGHT_DISABLE	asm("NOP")

#define PUMP_LEFT_ENABLE	asm("NOP")
#define PUMP_LEFT_DISABLE	asm("NOP")
#define PUMP_RIGHT_ENABLE	asm("NOP")
#define PUMP_RIGHT_DISABLE	asm("NOP")

#define MC_ENABLE		asm("NOP")
#define MC_DISABLE		asm("NOP")
#define MC_RIGHT_ENABLE		asm("NOP")
#define MC_RIGHT_DISABLE	asm("NOP")

#define TELEMETRY_ENABLE 	asm("NOP")
#define TELEMETRY_DISABLE 	asm("NOP")

#define AUX_ENABLE 			asm("NOP")
#define AUX_DISABLE 		asm("NOP")

#define DC_POWER_DISABLE 	asm("NOP")
#define DC_POWER_ENABLE		asm("NOP")

#define BRAKE_LIGHT_ENABLE 	asm("NOP")
#define BRAKE_LIGHT_DISABLE	asm("NOP")


#define WSB_ENABLE 	asm("NOP")
#define WSB_DISABLE	asm("NOP") 
#define BMU_ENABLE 	asm("NOP")
#define BMU_DISABLE asm("NOP")
#define VCU_ENABLE 	asm("NOP")
#define VCU_DISABLE asm("NOP")
#define DCU_ENABLE 	asm("NOP")
#define DCU_DISABLE	asm("NOP")


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

// Comment out to disable CLI printing
// To completely disable CLI, also remove call to uartStartReceiving
#define CONSOLE_PRINT_ON

#endif /* __BSP_H */
