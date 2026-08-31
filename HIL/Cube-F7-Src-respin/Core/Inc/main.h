/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#define LDAC_1_Pin GPIO_PIN_10
#define LDAC_1_GPIO_Port GPIOF
#define OSC_OUT_Pin GPIO_PIN_0
#define OSC_OUT_GPIO_Port GPIOH
#define OSC_IN_Pin GPIO_PIN_1
#define OSC_IN_GPIO_Port GPIOH
#define LDAC_2_Pin GPIO_PIN_1
#define LDAC_2_GPIO_Port GPIOC
#define LDAC_3_Pin GPIO_PIN_3
#define LDAC_3_GPIO_Port GPIOA
#define DAC_1_Pin GPIO_PIN_4
#define DAC_1_GPIO_Port GPIOA
#define DAC_2_Pin GPIO_PIN_5
#define DAC_2_GPIO_Port GPIOA
#define GPIO5V_8_Pin GPIO_PIN_4
#define GPIO5V_8_GPIO_Port GPIOC
#define GPIO5V_7_Pin GPIO_PIN_5
#define GPIO5V_7_GPIO_Port GPIOC
#define GPIO5V_6_Pin GPIO_PIN_0
#define GPIO5V_6_GPIO_Port GPIOB
#define GPIO5V_5_Pin GPIO_PIN_1
#define GPIO5V_5_GPIO_Port GPIOB
#define GPIO5V_4_Pin GPIO_PIN_2
#define GPIO5V_4_GPIO_Port GPIOB
#define GPIO5V_3_Pin GPIO_PIN_15
#define GPIO5V_3_GPIO_Port GPIOI
#define GPIO5V_2_Pin GPIO_PIN_0
#define GPIO5V_2_GPIO_Port GPIOJ
#define GPIO5V_1_Pin GPIO_PIN_1
#define GPIO5V_1_GPIO_Port GPIOJ
#define GPIO12V_9_Pin GPIO_PIN_2
#define GPIO12V_9_GPIO_Port GPIOJ
#define GPIO12V_8_Pin GPIO_PIN_3
#define GPIO12V_8_GPIO_Port GPIOJ
#define GPIO12V_7_Pin GPIO_PIN_4
#define GPIO12V_7_GPIO_Port GPIOJ
#define GPIO12V_6_Pin GPIO_PIN_11
#define GPIO12V_6_GPIO_Port GPIOF
#define GPIO12V_5_Pin GPIO_PIN_12
#define GPIO12V_5_GPIO_Port GPIOF
#define GPIO12V_4_Pin GPIO_PIN_13
#define GPIO12V_4_GPIO_Port GPIOF
#define GPIO12V_3_Pin GPIO_PIN_14
#define GPIO12V_3_GPIO_Port GPIOF
#define GPIO12V_2_Pin GPIO_PIN_15
#define GPIO12V_2_GPIO_Port GPIOF
#define GPIO12V_1_Pin GPIO_PIN_0
#define GPIO12V_1_GPIO_Port GPIOG
#define GPIO12V_10_Pin GPIO_PIN_1
#define GPIO12V_10_GPIO_Port GPIOG
#define GPIO12V_11_Pin GPIO_PIN_9
#define GPIO12V_11_GPIO_Port GPIOE
#define GPIO12V_12_Pin GPIO_PIN_10
#define GPIO12V_12_GPIO_Port GPIOE
#define GPIO12V_13_Pin GPIO_PIN_11
#define GPIO12V_13_GPIO_Port GPIOE
#define GPIO12V_14_Pin GPIO_PIN_12
#define GPIO12V_14_GPIO_Port GPIOE
#define GPIO12V_15_Pin GPIO_PIN_13
#define GPIO12V_15_GPIO_Port GPIOE
#define GPIO12V_16_Pin GPIO_PIN_14
#define GPIO12V_16_GPIO_Port GPIOE
#define GPIO12V_17_Pin GPIO_PIN_15
#define GPIO12V_17_GPIO_Port GPIOE
#define GPIO12V_18_Pin GPIO_PIN_9
#define GPIO12V_18_GPIO_Port GPIOH
#define GPIO12V_19_Pin GPIO_PIN_10
#define GPIO12V_19_GPIO_Port GPIOH
#define GPIO12V_20_Pin GPIO_PIN_11
#define GPIO12V_20_GPIO_Port GPIOH
#define GPIO12V_21_Pin GPIO_PIN_12
#define GPIO12V_21_GPIO_Port GPIOH
#define GPIO3V_1_Pin GPIO_PIN_12
#define GPIO3V_1_GPIO_Port GPIOB
#define GPIO3V_2_Pin GPIO_PIN_13
#define GPIO3V_2_GPIO_Port GPIOB
#define GPIO3V_3_Pin GPIO_PIN_14
#define GPIO3V_3_GPIO_Port GPIOB
#define GPIO3V_4_Pin GPIO_PIN_15
#define GPIO3V_4_GPIO_Port GPIOB
#define GPIO3V_5_Pin GPIO_PIN_8
#define GPIO3V_5_GPIO_Port GPIOD
#define GPIO3V_6_Pin GPIO_PIN_9
#define GPIO3V_6_GPIO_Port GPIOD
#define GPIO3V_7_Pin GPIO_PIN_10
#define GPIO3V_7_GPIO_Port GPIOD
#define GPIO3V_8_Pin GPIO_PIN_11
#define GPIO3V_8_GPIO_Port GPIOD
#define GPIO3V_9_Pin GPIO_PIN_12
#define GPIO3V_9_GPIO_Port GPIOD
#define GPIO3V_10_Pin GPIO_PIN_13
#define GPIO3V_10_GPIO_Port GPIOD
#define GPIO3V_11_Pin GPIO_PIN_14
#define GPIO3V_11_GPIO_Port GPIOD
#define GPIO3V_12_Pin GPIO_PIN_15
#define GPIO3V_12_GPIO_Port GPIOD
#define PWM_8_Pin GPIO_PIN_6
#define PWM_8_GPIO_Port GPIOC
#define PWM_9_Pin GPIO_PIN_7
#define PWM_9_GPIO_Port GPIOC
#define PWM_10_Pin GPIO_PIN_8
#define PWM_10_GPIO_Port GPIOC
#define GPIO3V_13_Pin GPIO_PIN_9
#define GPIO3V_13_GPIO_Port GPIOC
#define GPIO3V_14_Pin GPIO_PIN_9
#define GPIO3V_14_GPIO_Port GPIOA
#define GPIO3V_15_Pin GPIO_PIN_10
#define GPIO3V_15_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define LDAC_4_Pin GPIO_PIN_7
#define LDAC_4_GPIO_Port GPIOI

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
