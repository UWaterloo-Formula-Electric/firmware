/**
  *****************************************************************************
  * @file    sense.c
  * @author  Richard Matthews
  * @brief   Module containing task to monitor ADC values, mainly the rear
  * brake pressure
  *****************************************************************************
  */

#include "sense.h"
#include "bsp.h"
#include "bmu_can.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "watchdog.h"

/*#define SENSOR_TASK_PERIOD 50*/
#define SENSOR_TASK_PERIOD 1000

volatile uint32_t brakeAndHallAdcVals[BRAKE_HALL_ADC_CHANNEL_NUM] = {0};

HAL_StatusTypeDef sensorTaskInit()
{
#if IS_BOARD_F7
    if (HAL_ADC_Start_DMA(&BRAKE_ADC_HANDLE, (uint32_t *)brakeAndHallAdcVals, BRAKE_HALL_ADC_CHANNEL_NUM) != HAL_OK)
    {
        ERROR_PRINT("Failed to start Brake sensor ADC DMA conversions\n");
        Error_Handler();
        return HAL_ERROR;
    }

    if (HAL_TIM_Base_Start(&BRAKE_TIM_ADC_HANDLE) != HAL_OK)
    {
        ERROR_PRINT("Failed to start Brake sensor ADC DMA conversions\n");
        Error_Handler();
        return HAL_ERROR;
    }
#else
    // Init to full brake
    brakeAndHallAdcVals[BRAKE_HALL_ADC_CHANNEL_BRAKE] = 100*BRAKE_ADC_DIVIDER;
#endif

    return HAL_OK;
}

/**
 * Task to monitor ADC values, mainly the rear brake pressure
 */
void sensorTask(void *pvParameters)
{
    
    if (registerTaskToWatch(3, 2*pdMS_TO_TICKS(SENSOR_TASK_PERIOD), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register battery task with watchdog!\n");
        Error_Handler();
    }

    if (sensorTaskInit() != HAL_OK) {
       ERROR_PRINT("Failed to init sensor task\n");
       Error_Handler();
    }

   TickType_t xLastWakeTime = xTaskGetTickCount();
   while (1)
   {
      BrakePressureBMU = brakeAndHallAdcVals[BRAKE_HALL_ADC_CHANNEL_BRAKE] / BRAKE_ADC_DIVIDER;
      if (sendCAN_BMU_BrakePressure() != HAL_OK) {
         ERROR_PRINT("Failed to send brake value over CAN\n");
      }

      watchdogTaskCheckIn(3);
      vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSOR_TASK_PERIOD));
   }
}