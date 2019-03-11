#include "sense.h"
#include "bsp.h"
#include "BMU_can.h"
#include "debug.h"
#include "freertos.h"
#include "task.h"

#define SENSOR_TASK_PERIOD 50

volatile uint32_t brakeADCVal;

HAL_StatusTypeDef sensorTaskInit()
{
#if IS_BOARD_F7
    if (HAL_ADC_Start_DMA(&BRAKE_ADC_HANDLE, (uint32_t *)(&brakeADCVal), 1) != HAL_OK)
    {
        ERROR_PRINT("Failed to start Brake sensor ADC DMA conversions\n");
        Error_Handler();
        return HAL_ERROR;
    }
#else
    // Init to full brake
    brakeADCVal = 100*BRAKE_ADC_DIVIDER;
#endif

    return HAL_OK;
}

void sensorTask(void *pvParameters)
{
   while (1)
   {
      BrakePressureBMU = brakeADCVal / BRAKE_ADC_DIVIDER;
      if (sendCAN_BMU_BrakePressure() != HAL_OK) {
         ERROR_PRINT("Failed to send brake value over CAN\n");
      }
      vTaskDelay(SENSOR_TASK_PERIOD);
   }
}
