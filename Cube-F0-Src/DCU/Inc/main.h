/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include "stm32f0xx_hal.h"

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
void _Error_Handler(char *file, int line);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
//#define Error_Handler() junk
/* USER CODE END EM */

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_R_Pin GPIO_PIN_0
#define LED_R_GPIO_Port GPIOC
#define LED_Y_Pin GPIO_PIN_1
#define LED_Y_GPIO_Port GPIOC
#define LED_B_Pin GPIO_PIN_2
#define LED_B_GPIO_Port GPIOC
#define BTN_HV_READ_Pin GPIO_PIN_0
#define BTN_HV_READ_GPIO_Port GPIOA
#define BTN_HV_READ_EXTI_IRQn EXTI0_1_IRQn
#define BTN_EV_READ_Pin GPIO_PIN_1
#define BTN_EV_READ_GPIO_Port GPIOA
#define BTN_EV_READ_EXTI_IRQn EXTI0_1_IRQn
#define BTN_NAV_R_READ_Pin GPIO_PIN_4
#define BTN_NAV_R_READ_GPIO_Port GPIOA
#define BTN_NAV_L_READ_Pin GPIO_PIN_5
#define BTN_NAV_L_READ_GPIO_Port GPIOA
#define BTN_SELECT_READ_Pin GPIO_PIN_6
#define BTN_SELECT_READ_GPIO_Port GPIOA
#define IMD_LED_EN_Pin GPIO_PIN_0
#define IMD_LED_EN_GPIO_Port GPIOB
#define AMS_LED_RED_EN_Pin GPIO_PIN_1
#define AMS_LED_RED_EN_GPIO_Port GPIOB
#define BUZZER_EN_Pin GPIO_PIN_2
#define BUZZER_EN_GPIO_Port GPIOB
#define BTN_TC_READ_Pin GPIO_PIN_10
#define BTN_TC_READ_GPIO_Port GPIOB
#define BTN_TV_READ_Pin GPIO_PIN_11
#define BTN_TV_READ_GPIO_Port GPIOB
#define TC_LED_EN_Pin GPIO_PIN_12
#define TC_LED_EN_GPIO_Port GPIOB
#define TV_LED_EN_Pin GPIO_PIN_15
#define TV_LED_EN_GPIO_Port GPIOB
#define MC_LED_EN_Pin GPIO_PIN_6
#define MC_LED_EN_GPIO_Port GPIOC
#define BUT1_Pin GPIO_PIN_7
#define BUT1_GPIO_Port GPIOC
#define BUT2_Pin GPIO_PIN_8
#define BUT2_GPIO_Port GPIOC
#define BUT3_Pin GPIO_PIN_9
#define BUT3_GPIO_Port GPIOC
#define LVL_SHIFT_EN_Pin GPIO_PIN_12
#define LVL_SHIFT_EN_GPIO_Port GPIOA
#define LOW_BATT_LED_EN_Pin GPIO_PIN_2
#define LOW_BATT_LED_EN_GPIO_Port GPIOD
#define HV_LED_EN_Pin GPIO_PIN_3
#define HV_LED_EN_GPIO_Port GPIOB
#define EV_LED_EN_Pin GPIO_PIN_4
#define EV_LED_EN_GPIO_Port GPIOB
#define MOT_LED_RED_EN_Pin GPIO_PIN_5
#define MOT_LED_RED_EN_GPIO_Port GPIOB
#define MOT_LED_GR_EN_Pin GPIO_PIN_6
#define MOT_LED_GR_EN_GPIO_Port GPIOB
#define AMS_LED_GR_EN_Pin GPIO_PIN_7
#define AMS_LED_GR_EN_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
