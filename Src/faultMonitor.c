#include "faultMonitor.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "watchdog.h"
#include "FreeRTOS.h"
#include "task.h"

#define FAULT_MEASURE_TASK_PERIOD 100
#define FAULT_TASK_ID 6

HAL_StatusTypeDef HVIL_Control(bool enable)
{
   if (enable)
   {
      HAL_GPIO_WritePin(HVIL_EN_GPIO_Port, HVIL_EN_Pin, GPIO_PIN_SET);
   } else {
      HAL_GPIO_WritePin(HVIL_EN_GPIO_Port, HVIL_EN_Pin, GPIO_PIN_RESET);
   }

   return HAL_OK;
}

void faultMonitorTask(void *pvParameters)
{

   HVIL_Control(true);

   DEBUG_PRINT("Waiting for HVIL to start\n");
   do {
      vTaskDelay(10);
   } while (HAL_GPIO_ReadPin(HVIL_SENSE_GPIO_Port, HVIL_SENSE_Pin) == GPIO_PIN_RESET);

   fsmSendEvent(&fsmHandle, EV_HVIL_Ready, portMAX_DELAY);

   if (registerTaskToWatch(FAULT_TASK_ID, 2*pdMS_TO_TICKS(FAULT_MEASURE_TASK_PERIOD), false, NULL) != HAL_OK)
   {
     ERROR_PRINT("Failed to register fault monitor task with watchdog!\n");
     Error_Handler();
   }
   while (1)
   {
      if (HAL_GPIO_ReadPin(HVIL_SENSE_GPIO_Port, HVIL_SENSE_Pin) == GPIO_PIN_RESET)
      {
         ERROR_PRINT("HVIL broke\n");
         fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);

         while (1) {
            watchdogTaskCheckIn(FAULT_TASK_ID);
            vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
         }
      }

      watchdogTaskCheckIn(FAULT_TASK_ID);
      vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
   }
}
