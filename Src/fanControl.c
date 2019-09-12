#include "bsp.h"
#include "fanControl.h"
#include "BMU_can.h"
#include "batteries.h"
#include "debug.h"
#include "controlStateMachine.h"
#include "state_machine.h"

#define FAN_OFF_TEMP 30
// Fans need pwm of 25 kHz, so we set timer to have 10 MHz freq, and 400 period
#define FAN_MAX_DUTY_PERCENT 1.0
#define FAN_ON_DUTY_PERCENT 0.2
#define FAN_PERIOD_COUNT 400
#define FAN_TASK_PERIOD_MS 100

uint32_t calculateFanPeriod()
{
  // PWM Output is inverted from what we generate from PROC

  // Full fan while charging
  if (fsmGetState(&fsmHandle) == STATE_Charging) {
    /*DEBUG_PRINT("Charging fans\n");*/
    return FAN_PERIOD_COUNT*FAN_MAX_DUTY_PERCENT;
  }

  if (TempCellMax < FAN_OFF_TEMP) {
    return FAN_PERIOD_COUNT;
  }

  return FAN_PERIOD_COUNT
    - map_range_float(TempCellMax, FAN_OFF_TEMP, CELL_MAX_TEMP_C,
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
   int duty = calculateFanPeriod();

    __HAL_TIM_SET_COMPARE(&FAN_HANDLE, TIM_CHANNEL_1, duty);

    return HAL_OK;
}

void fanTask()
{
  if (fanInit() != HAL_OK) {
    ERROR_PRINT("Failed to init fans\n");
    Error_Handler();
  }

  while (1) {
    setFan();
    vTaskDelay(pdMS_TO_TICKS(FAN_TASK_PERIOD_MS));
  }
}
