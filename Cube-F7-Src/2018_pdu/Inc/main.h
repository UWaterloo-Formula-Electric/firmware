/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H__
#define __MAIN_H__

/* Includes ------------------------------------------------------------------*/

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

#define LED_B_Pin GPIO_PIN_13
#define LED_B_GPIO_Port GPIOC
#define BUT_1_Pin GPIO_PIN_14
#define BUT_1_GPIO_Port GPIOC
#define BUT_2_Pin GPIO_PIN_15
#define BUT_2_GPIO_Port GPIOC
#define CURRENT_MEASURE_Pin GPIO_PIN_0
#define CURRENT_MEASURE_GPIO_Port GPIOC
#define VOLTAGE_MEASURE_Pin GPIO_PIN_1
#define VOLTAGE_MEASURE_GPIO_Port GPIOC
#define MC_RIGHT_SENSE_Pin GPIO_PIN_2
#define MC_RIGHT_SENSE_GPIO_Port GPIOC
#define PUMP_RIGHT_SENSE_Pin GPIO_PIN_3
#define PUMP_RIGHT_SENSE_GPIO_Port GPIOC
#define FAN_RIGHT_SENSE_Pin GPIO_PIN_2
#define FAN_RIGHT_SENSE_GPIO_Port GPIOA
#define DCU_SENSE_Pin GPIO_PIN_3
#define DCU_SENSE_GPIO_Port GPIOA
#define MC_LEFT_SENSE_Pin GPIO_PIN_4
#define MC_LEFT_SENSE_GPIO_Port GPIOA
#define PUMP_LEFT_SENSE_Pin GPIO_PIN_5
#define PUMP_LEFT_SENSE_GPIO_Port GPIOA
#define FAN_LEFT_SENSE_Pin GPIO_PIN_6
#define FAN_LEFT_SENSE_GPIO_Port GPIOA
#define VCU_SENSE_Pin GPIO_PIN_7
#define VCU_SENSE_GPIO_Port GPIOA
#define BMU_SENSE_Pin GPIO_PIN_4
#define BMU_SENSE_GPIO_Port GPIOC
#define WSB_SENSE_Pin GPIO_PIN_5
#define WSB_SENSE_GPIO_Port GPIOC
#define TELEMETRY_SENSE_Pin GPIO_PIN_0
#define TELEMETRY_SENSE_GPIO_Port GPIOB
#define FAN_BATT_SENSE_Pin GPIO_PIN_1
#define FAN_BATT_SENSE_GPIO_Port GPIOB
#define BUT_3_Pin GPIO_PIN_15
#define BUT_3_GPIO_Port GPIOE
#define BMGR_DCOKn_Pin GPIO_PIN_8
#define BMGR_DCOKn_GPIO_Port GPIOA
#define BMGR_GPIO1_Pin GPIO_PIN_9
#define BMGR_GPIO1_GPIO_Port GPIOA
#define BMGR_GPIO3_Pin GPIO_PIN_11
#define BMGR_GPIO3_GPIO_Port GPIOA
#define BMGR_SHDN_Pin GPIO_PIN_12
#define BMGR_SHDN_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define FAN_BATT_PWM_Pin GPIO_PIN_15
#define FAN_BATT_PWM_GPIO_Port GPIOA
#define LED_R_Pin GPIO_PIN_11
#define LED_R_GPIO_Port GPIOC
#define LED_Y_Pin GPIO_PIN_12
#define LED_Y_GPIO_Port GPIOC
#define FAN_BATT_EN_Pin GPIO_PIN_0
#define FAN_BATT_EN_GPIO_Port GPIOD
#define TELEMETRY_EN_Pin GPIO_PIN_1
#define TELEMETRY_EN_GPIO_Port GPIOD
#define WSB_EN_Pin GPIO_PIN_2
#define WSB_EN_GPIO_Port GPIOD
#define BMU_EN_Pin GPIO_PIN_3
#define BMU_EN_GPIO_Port GPIOD
#define VCU_EN_Pin GPIO_PIN_4
#define VCU_EN_GPIO_Port GPIOD
#define FAN_LEFT_EN_Pin GPIO_PIN_5
#define FAN_LEFT_EN_GPIO_Port GPIOD
#define PUMP_LEFT_EN_Pin GPIO_PIN_6
#define PUMP_LEFT_EN_GPIO_Port GPIOD
#define MC_LEFT_EN_Pin GPIO_PIN_7
#define MC_LEFT_EN_GPIO_Port GPIOD
#define CAN_RX_Pin GPIO_PIN_3
#define CAN_RX_GPIO_Port GPIOB
#define CAN_TX_Pin GPIO_PIN_4
#define CAN_TX_GPIO_Port GPIOB
#define DCU_EN_Pin GPIO_PIN_5
#define DCU_EN_GPIO_Port GPIOB
#define FAN_RIGHT_EN_Pin GPIO_PIN_6
#define FAN_RIGHT_EN_GPIO_Port GPIOB
#define PUMP_RIGHT_EN_Pin GPIO_PIN_7
#define PUMP_RIGHT_EN_GPIO_Port GPIOB
#define MC_RIGHT_EN_Pin GPIO_PIN_8
#define MC_RIGHT_EN_GPIO_Port GPIOB

/* ########################## Assert Selection ############################## */
/**
  * @brief Uncomment the line below to expanse the "assert_param" macro in the 
  *        HAL drivers code
  */
 #define USE_FULL_ASSERT    1U 

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
 extern "C" {
#endif
void _Error_Handler(char *, int);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
