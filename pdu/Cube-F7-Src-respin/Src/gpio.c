/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, CURR_SENSE_MUX_S3_Pin|CURR_SENSE_MUX_S2_Pin|CURR_SENSE_MUX_S1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, I_DIAG_SENSE_SEL_Pin|I_DIAG_SENSE_EN_Pin|AUX_4_PWR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, AUX_3_PWR_Pin|AUX_2_PWR_Pin|AUX_1_PWR_Pin|RAD_EN_Pin
                          |INV_EN_Pin|ACC_FANS_EN_Pin|BRAKE_LIGHT_EN_Pin|TCU_EN_Pin
                          |LED_R_Pin|LED_Y_Pin|LED_B_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, WSB_EN_Pin|BMU_EN_Pin|CDU_EN_Pin|PUMP_2_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, PUMP_1_EN_Pin|CHARGE_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : CURR_SENSE_MUX_S3_Pin CURR_SENSE_MUX_S2_Pin CURR_SENSE_MUX_S1_Pin */
  GPIO_InitStruct.Pin = CURR_SENSE_MUX_S3_Pin|CURR_SENSE_MUX_S2_Pin|CURR_SENSE_MUX_S1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : I_DIAG_SENSE_SEL_Pin I_DIAG_SENSE_EN_Pin AUX_4_PWR_Pin */
  GPIO_InitStruct.Pin = I_DIAG_SENSE_SEL_Pin|I_DIAG_SENSE_EN_Pin|AUX_4_PWR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : AUX_3_PWR_Pin AUX_2_PWR_Pin AUX_1_PWR_Pin RAD_EN_Pin
                           INV_EN_Pin ACC_FANS_EN_Pin BRAKE_LIGHT_EN_Pin TCU_EN_Pin
                           LED_R_Pin LED_Y_Pin LED_B_Pin */
  GPIO_InitStruct.Pin = AUX_3_PWR_Pin|AUX_2_PWR_Pin|AUX_1_PWR_Pin|RAD_EN_Pin
                          |INV_EN_Pin|ACC_FANS_EN_Pin|BRAKE_LIGHT_EN_Pin|TCU_EN_Pin
                          |LED_R_Pin|LED_Y_Pin|LED_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : WSB_EN_Pin BMU_EN_Pin CDU_EN_Pin PUMP_2_EN_Pin */
  GPIO_InitStruct.Pin = WSB_EN_Pin|BMU_EN_Pin|CDU_EN_Pin|PUMP_2_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PUMP_1_EN_Pin CHARGE_EN_Pin */
  GPIO_InitStruct.Pin = PUMP_1_EN_Pin|CHARGE_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : CHG_STAT_1_Pin CHG_STAT_2_Pin TEMP_ALERT_Pin */
  GPIO_InitStruct.Pin = CHG_STAT_1_Pin|CHG_STAT_2_Pin|TEMP_ALERT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BUT3_Pin BUT2_Pin BUT1_Pin */
  GPIO_InitStruct.Pin = BUT3_Pin|BUT2_Pin|BUT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
