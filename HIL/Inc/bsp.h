/**
  *****************************************************************************
  * @file    bsp.h
  * @brief   Board support package (BSP) header file for the HIL bench-test
  *          board.
  * @details Instead of directly referring to hardware in source code, create
  * a define here and use that everywhere else, the same convention every
  * other board uses. HIL never ships in the car - see ../README.md.
  *
  ******************************************************************************
  */

#ifndef HIL_BSP_H
#define HIL_BSP_H

#include "boardTypes.h"
#include "main.h"
#include "can.h"
#include "usart.h"
#include "stdbool.h"

#if IS_BOARD_F7
#include "stm32f7xx_hal.h"
#include "dac.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"

/* Peripheral handles ------------------------------------------------------ */

// UART4 is the schematic's debug link but is pinned to floating PA0, see README
#define DEBUG_UART_HANDLE huart3
#define CAN_HANDLE hcan3            // Primary bench bus
#define CAN_1_HANDLE hcan1
#define CAN_2_HANDLE hcan2

// Signal injection, broken out to the bench connectors
#define DAC_HANDLE hdac
#define DAC_1_CHANNEL DAC_CHANNEL_1
#define DAC_2_CHANNEL DAC_CHANNEL_2
#define I2C_1_HANDLE hi2c1
#define I2C_2_HANDLE hi2c2
#define I2C_3_HANDLE hi2c3
#define SPI_4_HANDLE hspi4
#define SPI_5_HANDLE hspi5

#define PWM_TIM_HANDLE htim8        // PC6/PC7/PC8
#define PWM_8_CHANNEL TIM_CHANNEL_1
#define PWM_9_CHANNEL TIM_CHANNEL_2
#define PWM_10_CHANNEL TIM_CHANNEL_3
#define STATS_TIM_HANDLE htim7      // Owned by common/Src/debug.c

#define DEBUG_LED_PIN GPIO3V_1_Pin
#define DEBUG_LED_PORT GPIO3V_1_GPIO_Port
#define ERROR_LED_PIN GPIO3V_2_Pin
#define ERROR_LED_PORT GPIO3V_2_GPIO_Port

#define LDAC_1_LOW HAL_GPIO_WritePin(LDAC_1_GPIO_Port, LDAC_1_Pin, GPIO_PIN_RESET)
#define LDAC_1_HIGH HAL_GPIO_WritePin(LDAC_1_GPIO_Port, LDAC_1_Pin, GPIO_PIN_SET)
#define LDAC_2_LOW HAL_GPIO_WritePin(LDAC_2_GPIO_Port, LDAC_2_Pin, GPIO_PIN_RESET)
#define LDAC_2_HIGH HAL_GPIO_WritePin(LDAC_2_GPIO_Port, LDAC_2_Pin, GPIO_PIN_SET)
#define LDAC_3_LOW HAL_GPIO_WritePin(LDAC_3_GPIO_Port, LDAC_3_Pin, GPIO_PIN_RESET)
#define LDAC_3_HIGH HAL_GPIO_WritePin(LDAC_3_GPIO_Port, LDAC_3_Pin, GPIO_PIN_SET)
#define LDAC_4_LOW HAL_GPIO_WritePin(LDAC_4_GPIO_Port, LDAC_4_Pin, GPIO_PIN_RESET)
#define LDAC_4_HIGH HAL_GPIO_WritePin(LDAC_4_GPIO_Port, LDAC_4_Pin, GPIO_PIN_SET)

#define GPIO5V_WRITE(n, state) HAL_GPIO_WritePin(GPIO5V_##n##_GPIO_Port, GPIO5V_##n##_Pin, (state))
#define GPIO5V_READ(n) HAL_GPIO_ReadPin(GPIO5V_##n##_GPIO_Port, GPIO5V_##n##_Pin)
#define GPIO12V_WRITE(n, state) HAL_GPIO_WritePin(GPIO12V_##n##_GPIO_Port, GPIO12V_##n##_Pin, (state))
#define GPIO12V_READ(n) HAL_GPIO_ReadPin(GPIO12V_##n##_GPIO_Port, GPIO12V_##n##_Pin)
#define GPIO3V_WRITE(n, state) HAL_GPIO_WritePin(GPIO3V_##n##_GPIO_Port, GPIO3V_##n##_Pin, (state))
#define GPIO3V_READ(n) HAL_GPIO_ReadPin(GPIO3V_##n##_GPIO_Port, GPIO3V_##n##_Pin)

// TODO: PWM_1..PWM_7 (PG2-PG8) have no timer AF on this part, GPIO only

#else

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message "BOARD_TYPE_F7: " #BOARD_TYPE_F7
#error Compiling for unknown board type

#endif

// Comment out to remove debug printing
#define DEBUG_ON

// Comment out to remove error printing
#define ERROR_PRINT_ON

#define CONSOLE_PRINT_ON

#endif /* HIL_BSP_H */
