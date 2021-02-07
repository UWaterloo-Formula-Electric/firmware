/**
  *****************************************************************************
  * @file    faultMonitor.c
  * @author  Richard Matthews
  * @brief   Module monitoring status of IL and HVIL
  * @details Contains fault monitor task which monitors the status of the
  * interlock loop (IL) and high voltage interlock loop (HVIL) and reports an
  * error when either breaks. Also contains functions for getting the status of
  * various points along the IL.
  ******************************************************************************
  */

#include "faultMonitor.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "watchdog.h"
#include "FreeRTOS.h"
#include "task.h"

#define FAULT_MEASURE_TASK_PERIOD 100
#define FAULT_TASK_ID 6

#define ENABLE_IL_CHECKS

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

bool getBSPD_Status()
{
   return (HAL_GPIO_ReadPin(BSPD_SENSE_GPIO_Port, BSPD_SENSE_Pin) == GPIO_PIN_SET);
}

bool getTSMS_Status()
{
   return (HAL_GPIO_ReadPin(TSMS_SENSE_GPIO_Port, TSMS_SENSE_Pin) == GPIO_PIN_SET);
}

bool getHVD_Status()
{
   return (HAL_GPIO_ReadPin(HVD_SENSE_GPIO_Port, HVD_SENSE_Pin) == GPIO_PIN_SET);
}

bool getHVIL_Status()
{
   return (HAL_GPIO_ReadPin(HVIL_SENSE_GPIO_Port, HVIL_SENSE_Pin) == GPIO_PIN_SET);
}

// IL in to the BMU
bool getIL_BRB_Status()
{
   return (HAL_GPIO_ReadPin(IL_SENSE_GPIO_Port, IL_SENSE_Pin) == GPIO_PIN_SET);
}

// IL in to the BMU
bool getIL_Status()
{
   return (HAL_GPIO_ReadPin(TSMS_SENSE_GPIO_Port, TSMS_SENSE_Pin) == GPIO_PIN_SET);
}

void faultMonitorTask(void *pvParameters)
{

#ifdef ENABLE_IL_CHECKS

   HVIL_Control(true);

   DEBUG_PRINT("Waiting for HVIL to start\n");
   do {
      vTaskDelay(10);
   } while (!getHVIL_Status());

   DEBUG_PRINT("HVIL Started\n");

   if (getIL_BRB_Status() == false) {
       DEBUG_PRINT("IL_BRB is down\n");
   }

   if (getBSPD_Status() == false) {
       DEBUG_PRINT("BSPD is down\n");
   }

   if (getHVD_Status() == false) {
       DEBUG_PRINT("HVD is down\n");
   }

   DEBUG_PRINT("Waiting for IL OK\n");
   do {
      vTaskDelay(10);
   } while (!getIL_Status());

   DEBUG_PRINT("IL Started\n");

   fsmSendEvent(&fsmHandle, EV_FaultMonitorReady, portMAX_DELAY);

   if (registerTaskToWatch(FAULT_TASK_ID, 2*pdMS_TO_TICKS(FAULT_MEASURE_TASK_PERIOD), false, NULL) != HAL_OK)
   {
     ERROR_PRINT("Failed to register fault monitor task with watchdog!\n");
     Error_Handler();
   }
   while (1)
   {
      if (!getHVIL_Status())
      {
         ERROR_PRINT("HVIL broke\n");
         fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);

         while (1) {
            watchdogTaskCheckIn(FAULT_TASK_ID);
            vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
         }
      }

      if (!getIL_Status())
      {
         ERROR_PRINT("IL broke\n");
         fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);

         while (1) {
            watchdogTaskCheckIn(FAULT_TASK_ID);
            vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
         }
      }

      watchdogTaskCheckIn(FAULT_TASK_ID);
      vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
   }

#else

   fsmSendEvent(&fsmHandle, EV_FaultMonitorReady, portMAX_DELAY);

   while (1) {
      vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
   }

#endif
}
