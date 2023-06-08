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
#define LED_R_Pin GPIO_PIN_0
#define LED_R_GPIO_Port GPIOC
#define LED_Y_Pin GPIO_PIN_1
#define LED_Y_GPIO_Port GPIOC
#define LED_B_Pin GPIO_PIN_2
#define LED_B_GPIO_Port GPIOC
#define Throttle_A_Pin GPIO_PIN_4
#define Throttle_A_GPIO_Port GPIOA
#define Throttle_B_Pin GPIO_PIN_5
#define Throttle_B_GPIO_Port GPIOA
#define Brake_Pos_Pin GPIO_PIN_6
#define Brake_Pos_GPIO_Port GPIOA
#define Steering_Pin GPIO_PIN_7
#define Steering_GPIO_Port GPIOA
#define Break_Pres_Pin GPIO_PIN_0
#define Break_Pres_GPIO_Port GPIOB
#define PP_BB_EN_Pin GPIO_PIN_8
#define PP_BB_EN_GPIO_Port GPIOD
#define PP_5V0_EN_Pin GPIO_PIN_9
#define PP_5V0_EN_GPIO_Port GPIOD
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
