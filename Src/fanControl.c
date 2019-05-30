#include "bsp.h"
#include "fanControl.h"
#include "BMU_can.h"
#include "batteries.h"
#include "debug.h"

#define FAN_OFF_TEMP 25
// Fans need pwm of 25 kHz, so we set timer to have 10 MHz freq, and 400 period
#define FAN_PERIOD_COUNT 400
#define FAN_TASK_PERIOD_MS 100

uint32_t calculateFanPeriod()
{
   return map_range_float(TempCellMax, FAN_OFF_TEMP, CELL_MAX_TEMP_C, 0, FAN_PERIOD_COUNT);
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
   /*int duty = calculateFanPeriod();*/
   int duty = 200;

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
