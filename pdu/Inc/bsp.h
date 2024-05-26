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

#define CAN_ENABLED

#define FAN_LEFT_ENABLE HAL_GPIO_WritePin(FAN_LEFT_EN_GPIO_Port,FAN_LEFT_EN_Pin,GPIO_PIN_SET)
#define FAN_LEFT_DISABLE HAL_GPIO_WritePin(FAN_LEFT_EN_GPIO_Port,FAN_LEFT_EN_Pin,GPIO_PIN_RESET)
#define FAN_RIGHT_ENABLE HAL_GPIO_WritePin(FAN_RIGHT_EN_GPIO_Port,FAN_RIGHT_EN_Pin,GPIO_PIN_SET)
#define FAN_RIGHT_DISABLE HAL_GPIO_WritePin(FAN_RIGHT_EN_GPIO_Port,FAN_RIGHT_EN_Pin,GPIO_PIN_RESET)
#define PUMP_LEFT_ENABLE HAL_GPIO_WritePin(PUMP_LEFT_EN_GPIO_Port,PUMP_LEFT_EN_Pin,GPIO_PIN_SET)
#define PUMP_LEFT_DISABLE HAL_GPIO_WritePin(PUMP_LEFT_EN_GPIO_Port,PUMP_LEFT_EN_Pin,GPIO_PIN_RESET)
#define PUMP_RIGHT_ENABLE HAL_GPIO_WritePin(PUMP_RIGHT_EN_GPIO_Port,PUMP_RIGHT_EN_Pin,GPIO_PIN_SET)
#define PUMP_RIGHT_DISABLE HAL_GPIO_WritePin(PUMP_RIGHT_EN_GPIO_Port,PUMP_RIGHT_EN_Pin,GPIO_PIN_RESET)
#define MC_ENABLE HAL_GPIO_WritePin(MC_EN_GPIO_Port,MC_EN_Pin,GPIO_PIN_SET)
#define MC_DISABLE HAL_GPIO_WritePin(MC_EN_GPIO_Port,MC_EN_Pin,GPIO_PIN_RESET)
#define MC_RIGHT_ENABLE HAL_GPIO_WritePin(MC_RIGHT_EN_GPIO_Port,MC_RIGHT_EN_Pin,GPIO_PIN_SET)
#define MC_RIGHT_DISABLE HAL_GPIO_WritePin(MC_RIGHT_EN_GPIO_Port,MC_RIGHT_EN_Pin,GPIO_PIN_RESET)
#define WSB_ENABLE HAL_GPIO_WritePin(WSB_EN_GPIO_Port,WSB_EN_Pin,GPIO_PIN_SET)
#define WSB_DISABLE HAL_GPIO_WritePin(WSB_EN_GPIO_Port,WSB_EN_Pin,GPIO_PIN_RESET)
#define BMU_ENABLE HAL_GPIO_WritePin(BMU_EN_GPIO_Port,BMU_EN_Pin,GPIO_PIN_SET)
#define BMU_DISABLE HAL_GPIO_WritePin(BMU_EN_GPIO_Port,BMU_EN_Pin,GPIO_PIN_RESET)
#define VCU_ENABLE HAL_GPIO_WritePin(VCU_EN_GPIO_Port,VCU_EN_Pin,GPIO_PIN_SET)
#define VCU_DISABLE HAL_GPIO_WritePin(VCU_EN_GPIO_Port,VCU_EN_Pin,GPIO_PIN_RESET)
#define DCU_ENABLE HAL_GPIO_WritePin(DCU_EN_GPIO_Port,DCU_EN_Pin,GPIO_PIN_SET)
#define DCU_DISABLE HAL_GPIO_WritePin(DCU_EN_GPIO_Port,DCU_EN_Pin,GPIO_PIN_RESET)

#define CHECK_DC_DC_ON_PIN		(HAL_GPIO_ReadPin(BMGR_DCOKn_GPIO_Port, BMGR_DCOKn_Pin) == GPIO_PIN_RESET)
#define CHECK_BMGR_GPIO1_PIN_STATE	(HAL_GPIO_ReadPin(BMGR_GPIO1_GPIO_Port, BMGR_GPIO1_Pin) == GPIO_PIN_SET)
#define CHECK_BMGR_GPIO2_PIN_STATE	(HAL_GPIO_ReadPin(BMGR_GPIO2_GPIO_Port, BMGR_GPIO2_Pin) == GPIO_PIN_SET)
#define CHECK_BMGR_GPIO3_PIN_STATE	(HAL_GPIO_ReadPin(BMGR_GPIO3_GPIO_Port, BMGR_GPIO3_Pin) == GPIO_PIN_SET)

#define DC_POWER_DISABLE HAL_GPIO_WritePin(BMGR_SHDN_GPIO_Port,BMGR_SHDN_Pin,GPIO_PIN_SET)
#define DC_POWER_ENABLE HAL_GPIO_WritePin(BMGR_SHDN_GPIO_Port,BMGR_SHDN_Pin,GPIO_PIN_RESET)

#define TELEMETRY_ENABLE 	asm("NOP")
#define TELEMETRY_DISABLE 	asm("NOP")

#define AUX_ENABLE 	HAL_GPIO_WritePin(AUX_EN_GPIO_Port,AUX_EN_Pin,GPIO_PIN_SET)
#define AUX_DISABLE HAL_GPIO_WritePin(AUX_EN_GPIO_Port,AUX_EN_Pin,GPIO_PIN_RESET)

#define AUX_ENABLE 	HAL_GPIO_WritePin(AUX_EN_GPIO_Port,AUX_EN_Pin,GPIO_PIN_SET)
#define AUX_DISABLE HAL_GPIO_WritePin(AUX_EN_GPIO_Port,AUX_EN_Pin,GPIO_PIN_RESET)

#define BRAKE_LIGHT_ENABLE HAL_GPIO_WritePin(BRAKE_LIGHT_EN_GPIO_Port,BRAKE_LIGHT_EN_Pin,GPIO_PIN_SET)
#define BRAKE_LIGHT_DISABLE HAL_GPIO_WritePin(BRAKE_LIGHT_EN_GPIO_Port,BRAKE_LIGHT_EN_Pin,GPIO_PIN_RESET)



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
