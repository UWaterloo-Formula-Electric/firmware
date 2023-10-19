/**
  *****************************************************************************
  * @file    fanControl.c
  * @author  Richard Matthews
  * @brief   Module controlling battery pack fans.
  * @details The battery pack contains an array of fans to cool the battery.
  * The BMU controls the fans based on the temperature of the battery cells,
  * turning them on as the cells heat up and increasing the fan speed
  * proportional to temeprature.
  ******************************************************************************
  */

#include "bsp.h"
#include "fanControl.h"
#include "bmu_can.h"
#include "batteries.h"
#include "debug.h"
#include "controlStateMachine.h"
#include "state_machine.h"

#define FAN_OFF_TEMP 25
#define FAN_PEAK_TEMP 35
// Fans need pwm of 25 kHz, so we set timer to have 10 MHz freq, and 400 period
#define FAN_MAX_DUTY_PERCENT 1.0
#define FAN_ON_DUTY_PERCENT 0.2
#define FAN_PERIOD_COUNT 400
#define FAN_TASK_PERIOD_MS 1000

uint32_t calculateFanPeriod()
{
  // PWM Output is inverted from what we generate from PROC
  // Full fan while charging
  if (fsmGetState(&fsmHandle) == STATE_Charging || fsmGetState(&fsmHandle) == STATE_Balancing) {
    /*DEBUG_PRINT("Charging fans\n");*/
    return FAN_PERIOD_COUNT - FAN_PERIOD_COUNT*FAN_MAX_DUTY_PERCENT;
  }

  if (TempCellMax < FAN_OFF_TEMP) {
    return FAN_PERIOD_COUNT;
  }

  return FAN_PERIOD_COUNT - map_range_float(TempCellMax, FAN_OFF_TEMP, FAN_PEAK_TEMP,
                      FAN_PERIOD_COUNT*FAN_ON_DUTY_PERCENT,
                      FAN_PERIOD_COUNT*FAN_MAX_DUTY_PERCENT);
}

HAL_StatusTypeDef fanInit()
{
  __HAL_TIM_SET_COMPARE(&FAN_HANDLE, TIM_CHANNEL_1, 0);

  if (HAL_TIM_PWM_Start(&FAN_HANDLE, TIM_CHANNEL_1) != HAL_OK)
  {
     ERROR_PRINT("Failed to start fan pwm timer\n");
     return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef setFan()
{
  uint32_t duty = calculateFanPeriod();

  __HAL_TIM_SET_COMPARE(&FAN_HANDLE, TIM_CHANNEL_1, duty);
  
  FanPeriod = duty;
  sendCAN_BMU_FanPeriod();
  return HAL_OK;
}

/**
 * Task to control the battery box fans.
 */
void fanTask()
{
  if (fanInit() != HAL_OK) {
    ERROR_PRINT("Failed to init fans\n");
    Error_Handler();
  }

  TickType_t xLastWakeTime = xTaskGetTickCount();
  while (1) {
    //setFan();
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(FAN_TASK_PERIOD_MS));
  }
}
