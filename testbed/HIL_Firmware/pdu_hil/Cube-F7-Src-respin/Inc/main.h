/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#define LDAC_Pin GPIO_PIN_10
#define LDAC_GPIO_Port GPIOF
#define PWR_AUX_Pin GPIO_PIN_1
#define PWR_AUX_GPIO_Port GPIOG
#define PWR_RIGHT_PUMP_Pin GPIO_PIN_9
#define PWR_RIGHT_PUMP_GPIO_Port GPIOE
#define PWR_LEFT_PUMP_Pin GPIO_PIN_10
#define PWR_LEFT_PUMP_GPIO_Port GPIOE
#define PWR_RIGHT_FAN_Pin GPIO_PIN_11
#define PWR_RIGHT_FAN_GPIO_Port GPIOE
#define PWR_LEFT_FAN_Pin GPIO_PIN_12
#define PWR_LEFT_FAN_GPIO_Port GPIOE
#define PWR_RIGHT_MC_Pin GPIO_PIN_13
#define PWR_RIGHT_MC_GPIO_Port GPIOE
#define PWR_LEFT_MC_Pin GPIO_PIN_14
#define PWR_LEFT_MC_GPIO_Port GPIOE
#define PWR_BRAKELIGHT_Pin GPIO_PIN_15
#define PWR_BRAKELIGHT_GPIO_Port GPIOE
#define PWR_RAD_Pin GPIO_PIN_9
#define PWR_RAD_GPIO_Port GPIOH
#define PWR_BATT_RAW_Pin GPIO_PIN_10
#define PWR_BATT_RAW_GPIO_Port GPIOH
#define PWR_BMU_Pin GPIO_PIN_11
#define PWR_BMU_GPIO_Port GPIOH
#define PWR_VCU_Pin GPIO_PIN_12
#define PWR_VCU_GPIO_Port GPIOH

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
