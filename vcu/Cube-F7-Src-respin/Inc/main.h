/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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
#define BUT1_Pin GPIO_PIN_13
#define BUT1_GPIO_Port GPIOC
#define BUT2_Pin GPIO_PIN_14
#define BUT2_GPIO_Port GPIOC
#define BUT3_Pin GPIO_PIN_15
#define BUT3_GPIO_Port GPIOC
#define Throttle_A_Pin GPIO_PIN_3
#define Throttle_A_GPIO_Port GPIOA
#define Throttle_B_Pin GPIO_PIN_4
#define Throttle_B_GPIO_Port GPIOA
#define Brake_Position_Pin GPIO_PIN_5
#define Brake_Position_GPIO_Port GPIOA
#define Brake_Pressure_Pin GPIO_PIN_6
#define Brake_Pressure_GPIO_Port GPIOA
#define Steering_Pin GPIO_PIN_7
#define Steering_GPIO_Port GPIOA
#define BTN_HV_SNS_Pin GPIO_PIN_4
#define BTN_HV_SNS_GPIO_Port GPIOC
#define BTN_EM_SNS_Pin GPIO_PIN_5
#define BTN_EM_SNS_GPIO_Port GPIOC
#define BTN_TC_SNS_Pin GPIO_PIN_0
#define BTN_TC_SNS_GPIO_Port GPIOB
#define BTN_ENDUR_SNS_Pin GPIO_PIN_1
#define BTN_ENDUR_SNS_GPIO_Port GPIOB
#define IMD_FAULT_LED_EN_Pin GPIO_PIN_8
#define IMD_FAULT_LED_EN_GPIO_Port GPIOD
#define AMS_FAULT_LED_EN_Pin GPIO_PIN_9
#define AMS_FAULT_LED_EN_GPIO_Port GPIOD
#define MC_FAULT_LED_EN_Pin GPIO_PIN_10
#define MC_FAULT_LED_EN_GPIO_Port GPIOD
#define MOT_FAULT_LED_EN_Pin GPIO_PIN_11
#define MOT_FAULT_LED_EN_GPIO_Port GPIOD
#define BUZZER_EN_Pin GPIO_PIN_14
#define BUZZER_EN_GPIO_Port GPIOD
#define LED_BTN_HV_EN_Pin GPIO_PIN_6
#define LED_BTN_HV_EN_GPIO_Port GPIOC
#define LED_BTN_EM_EN_Pin GPIO_PIN_7
#define LED_BTN_EM_EN_GPIO_Port GPIOC
#define LED_BTN_TC_EN_Pin GPIO_PIN_8
#define LED_BTN_TC_EN_GPIO_Port GPIOC
#define LED_BTN_ENDUR_EN_Pin GPIO_PIN_9
#define LED_BTN_ENDUR_EN_GPIO_Port GPIOC
#define UART_DBG2MCU_Pin GPIO_PIN_10
#define UART_DBG2MCU_GPIO_Port GPIOC
#define UART_MCU2DBG_Pin GPIO_PIN_11
#define UART_MCU2DBG_GPIO_Port GPIOC
#define DEBUG_BTN_1_Pin GPIO_PIN_2
#define DEBUG_BTN_1_GPIO_Port GPIOD
#define LED_R_Pin GPIO_PIN_3
#define LED_R_GPIO_Port GPIOD
#define LED_Y_Pin GPIO_PIN_4
#define LED_Y_GPIO_Port GPIOD
#define LED_B_Pin GPIO_PIN_5
#define LED_B_GPIO_Port GPIOD
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
