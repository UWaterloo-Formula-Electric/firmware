/* USER CODE BEGIN Header */
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CURR_MUX_OUT_Pin GPIO_PIN_3
#define CURR_MUX_OUT_GPIO_Port GPIOA
#define I_MAIN_Pin GPIO_PIN_4
#define I_MAIN_GPIO_Port GPIOA
#define V_MAIN_Pin GPIO_PIN_5
#define V_MAIN_GPIO_Port GPIOA
#define V_LIPO_Pin GPIO_PIN_6
#define V_LIPO_GPIO_Port GPIOA
#define I_DCDC_Pin GPIO_PIN_7
#define I_DCDC_GPIO_Port GPIOA
#define V_DCDC_Pin GPIO_PIN_4
#define V_DCDC_GPIO_Port GPIOC
#define LIPO_THERM_Pin GPIO_PIN_5
#define LIPO_THERM_GPIO_Port GPIOC
#define CURR_SENSE_MUX_S3_Pin GPIO_PIN_8
#define CURR_SENSE_MUX_S3_GPIO_Port GPIOE
#define CURR_SENSE_MUX_S2_Pin GPIO_PIN_9
#define CURR_SENSE_MUX_S2_GPIO_Port GPIOE
#define CURR_SENSE_MUX_S1_Pin GPIO_PIN_10
#define CURR_SENSE_MUX_S1_GPIO_Port GPIOE
#define I_DIAG_SENSE_SEL_Pin GPIO_PIN_13
#define I_DIAG_SENSE_SEL_GPIO_Port GPIOB
#define I_DIAG_SENSE_EN_Pin GPIO_PIN_14
#define I_DIAG_SENSE_EN_GPIO_Port GPIOB
#define AUX_4_PWR_Pin GPIO_PIN_15
#define AUX_4_PWR_GPIO_Port GPIOB
#define AUX_3_PWR_Pin GPIO_PIN_8
#define AUX_3_PWR_GPIO_Port GPIOD
#define AUX_2_PWR_Pin GPIO_PIN_9
#define AUX_2_PWR_GPIO_Port GPIOD
#define AUX_1_PWR_Pin GPIO_PIN_10
#define AUX_1_PWR_GPIO_Port GPIOD
#define RAD_EN_Pin GPIO_PIN_11
#define RAD_EN_GPIO_Port GPIOD
#define INV_EN_Pin GPIO_PIN_12
#define INV_EN_GPIO_Port GPIOD
#define ACC_FANS_EN_Pin GPIO_PIN_13
#define ACC_FANS_EN_GPIO_Port GPIOD
#define BRAKE_LIGHT_EN_Pin GPIO_PIN_14
#define BRAKE_LIGHT_EN_GPIO_Port GPIOD
#define TCU_EN_Pin GPIO_PIN_15
#define TCU_EN_GPIO_Port GPIOD
#define WSB_EN_Pin GPIO_PIN_6
#define WSB_EN_GPIO_Port GPIOC
#define BMU_EN_Pin GPIO_PIN_7
#define BMU_EN_GPIO_Port GPIOC
#define CDU_EN_Pin GPIO_PIN_8
#define CDU_EN_GPIO_Port GPIOC
#define PUMP_2_EN_Pin GPIO_PIN_9
#define PUMP_2_EN_GPIO_Port GPIOC
#define PUMP_1_EN_Pin GPIO_PIN_8
#define PUMP_1_EN_GPIO_Port GPIOA
#define CHARGE_EN_Pin GPIO_PIN_9
#define CHARGE_EN_GPIO_Port GPIOA
#define CHG_STAT_1_Pin GPIO_PIN_10
#define CHG_STAT_1_GPIO_Port GPIOA
#define CHG_STAT_2_Pin GPIO_PIN_11
#define CHG_STAT_2_GPIO_Port GPIOA
#define TEMP_ALERT_Pin GPIO_PIN_12
#define TEMP_ALERT_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define BUT3_Pin GPIO_PIN_0
#define BUT3_GPIO_Port GPIOD
#define BUT2_Pin GPIO_PIN_1
#define BUT2_GPIO_Port GPIOD
#define BUT1_Pin GPIO_PIN_2
#define BUT1_GPIO_Port GPIOD
#define LED_R_Pin GPIO_PIN_3
#define LED_R_GPIO_Port GPIOD
#define LED_Y_Pin GPIO_PIN_4
#define LED_Y_GPIO_Port GPIOD
#define LED_B_Pin GPIO_PIN_5
#define LED_B_GPIO_Port GPIOD
#define CAN_RX_Pin GPIO_PIN_3
#define CAN_RX_GPIO_Port GPIOB
#define CAN_TX_Pin GPIO_PIN_4
#define CAN_TX_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
