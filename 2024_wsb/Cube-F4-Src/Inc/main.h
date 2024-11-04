/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32f4xx_hal.h"

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
#define LeftRight_DipSW_Pin GPIO_PIN_4
#define LeftRight_DipSW_GPIO_Port GPIOE
#define FrontRear_DipSW_Pin GPIO_PIN_5
#define FrontRear_DipSW_GPIO_Port GPIOE
#define DP83848CVV_nRESET_Pin GPIO_PIN_14
#define DP83848CVV_nRESET_GPIO_Port GPIOC
#define SENSOR3_ANALOG_IN_Pin GPIO_PIN_0
#define SENSOR3_ANALOG_IN_GPIO_Port GPIOC
#define SENSOR1_ANALOG_IN_Pin GPIO_PIN_4
#define SENSOR1_ANALOG_IN_GPIO_Port GPIOA
#define DP83848CVV_RX_ER_Pin GPIO_PIN_5
#define DP83848CVV_RX_ER_GPIO_Port GPIOA
#define SENSOR2_ANALOG_IN_Pin GPIO_PIN_6
#define SENSOR2_ANALOG_IN_GPIO_Port GPIOA
#define HAL_EFFECT_Pin GPIO_PIN_13
#define HAL_EFFECT_GPIO_Port GPIOE
#define FW_HEARTBEAT_Pin GPIO_PIN_14
#define FW_HEARTBEAT_GPIO_Port GPIOB
#define DP83848CVV_INT_Pin GPIO_PIN_9
#define DP83848CVV_INT_GPIO_Port GPIOD
#define UART_MCU2DEBUG_Pin GPIO_PIN_9
#define UART_MCU2DEBUG_GPIO_Port GPIOA
#define UART_DEBUG2MCU_Pin GPIO_PIN_10
#define UART_DEBUG2MCU_GPIO_Port GPIOA
#define SPI1_CS1_Pin GPIO_PIN_6
#define SPI1_CS1_GPIO_Port GPIOB
#define SPI1_CS2_Pin GPIO_PIN_7
#define SPI1_CS2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
