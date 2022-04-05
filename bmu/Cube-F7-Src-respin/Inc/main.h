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
#define CONT_NEG_SENSE_Pin GPIO_PIN_0
#define CONT_NEG_SENSE_GPIO_Port GPIOA
#define CONT_POS_SENSE_Pin GPIO_PIN_1
#define CONT_POS_SENSE_GPIO_Port GPIOA
#define HV_ADC_SPI_NSS_Pin GPIO_PIN_4
#define HV_ADC_SPI_NSS_GPIO_Port GPIOA
#define HV_ADC_SPI_SCLK_Pin GPIO_PIN_5
#define HV_ADC_SPI_SCLK_GPIO_Port GPIOA
#define HV_ADC_SPI_MISO_Pin GPIO_PIN_6
#define HV_ADC_SPI_MISO_GPIO_Port GPIOA
#define HV_ADC_MOSI_Pin GPIO_PIN_7
#define HV_ADC_MOSI_GPIO_Port GPIOA
#define HV_ADC_DREADY_L_Pin GPIO_PIN_4
#define HV_ADC_DREADY_L_GPIO_Port GPIOC
#define BRAKE_SENSE_ANALOG_Pin GPIO_PIN_0
#define BRAKE_SENSE_ANALOG_GPIO_Port GPIOB
#define BUT1_Pin GPIO_PIN_1
#define BUT1_GPIO_Port GPIOB
#define BUT2_Pin GPIO_PIN_2
#define BUT2_GPIO_Port GPIOB
#define BUT3_Pin GPIO_PIN_7
#define BUT3_GPIO_Port GPIOE
#define LED_R_Pin GPIO_PIN_8
#define LED_R_GPIO_Port GPIOE
#define LED_Y_Pin GPIO_PIN_9
#define LED_Y_GPIO_Port GPIOE
#define LED_B_Pin GPIO_PIN_10
#define LED_B_GPIO_Port GPIOE
#define ISO_SPI_NSS_Pin GPIO_PIN_11
#define ISO_SPI_NSS_GPIO_Port GPIOE
#define ISO_SPI_SCLK_Pin GPIO_PIN_12
#define ISO_SPI_SCLK_GPIO_Port GPIOE
#define ISO_SPI_MISO_Pin GPIO_PIN_13
#define ISO_SPI_MISO_GPIO_Port GPIOE
#define ISO_SPI_MOSI_Pin GPIO_PIN_14
#define ISO_SPI_MOSI_GPIO_Port GPIOE
#define IL_SENSE_Pin GPIO_PIN_15
#define IL_SENSE_GPIO_Port GPIOE
#define BSPD_SENSE_Pin GPIO_PIN_11
#define BSPD_SENSE_GPIO_Port GPIOB
#define HVIL_SENSE_Pin GPIO_PIN_12
#define HVIL_SENSE_GPIO_Port GPIOB
#define HVIL_EN_Pin GPIO_PIN_13
#define HVIL_EN_GPIO_Port GPIOB
#define FAN_Pin GPIO_PIN_14
#define FAN_GPIO_Port GPIOB
#define HVD_SENSE_Pin GPIO_PIN_8
#define HVD_SENSE_GPIO_Port GPIOD
#define TACH_5_Pin GPIO_PIN_12
#define TACH_5_GPIO_Port GPIOD
#define TACH_4_Pin GPIO_PIN_13
#define TACH_4_GPIO_Port GPIOD
#define TACH_3_Pin GPIO_PIN_14
#define TACH_3_GPIO_Port GPIOD
#define TACH_2_Pin GPIO_PIN_15
#define TACH_2_GPIO_Port GPIOD
#define TACH_1_Pin GPIO_PIN_6
#define TACH_1_GPIO_Port GPIOC
#define IMD_STATUS_Pin GPIO_PIN_7
#define IMD_STATUS_GPIO_Port GPIOC
#define IMD_SENSE_Pin GPIO_PIN_8
#define IMD_SENSE_GPIO_Port GPIOC
#define COCKPIT_BRB_SENSE_Pin GPIO_PIN_0
#define COCKPIT_BRB_SENSE_GPIO_Port GPIOD
#define CONT_PRE_Pin GPIO_PIN_1
#define CONT_PRE_GPIO_Port GPIOD
#define AMS_CONT_Pin GPIO_PIN_2
#define AMS_CONT_GPIO_Port GPIOD
#define CONT_CHARGE_Pin GPIO_PIN_3
#define CONT_CHARGE_GPIO_Port GPIOD
#define CONT_POS_Pin GPIO_PIN_4
#define CONT_POS_GPIO_Port GPIOD
#define CONT_DC_DC_Pin GPIO_PIN_5
#define CONT_DC_DC_GPIO_Port GPIOD
#define CONT_NEG_Pin GPIO_PIN_6
#define CONT_NEG_GPIO_Port GPIOD
#define TSMS_SENSE_Pin GPIO_PIN_7
#define TSMS_SENSE_GPIO_Port GPIOD
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
