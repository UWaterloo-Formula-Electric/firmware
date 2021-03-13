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

   /* HVIL status */
   HVIL_Control(true);

   if (getHVIL_Status() == false)
   {
       DEBUG_PRINT("Fault Monitor: HVIL is down!\n");
       DEBUG_PRINT("Fault Monitor: Waiting for HVIL OK.\n");
   }

   while (getHVIL_Status() == false)
   {
      vTaskDelay(10);
   }

   DEBUG_PRINT("Fault Monitor: HVIL Started.\n");

   /* BRB Status */
   if (getIL_BRB_Status() == false)
   {
       DEBUG_PRINT("Fault Monitor: BRB is down!\n");
       DEBUG_PRINT("Fault Monitor: Waiting for BRB OK.\n");
       DEBUG_PRINT("Fault Monitor: -- help --\n");
       DEBUG_PRINT("Fault Monitor: This is IL_A in the 2019_BMU schematic.\n");
       DEBUG_PRINT("Fault Monitor: Things to check:\n");
       DEBUG_PRINT("Fault Monitor:  * Left, right and dash BRBs are closed.\n");
       DEBUG_PRINT("Fault Monitor:  * Brake overtravel switch is closed.\n");
       DEBUG_PRINT("Fault Monitor:  * Intertia/crash switch is closed.\n");
   }

   while (getIL_BRB_Status() == false)
   {
      vTaskDelay(10);
   }

   DEBUG_PRINT("Fault Monitor: BRB IL OK.\n");

   /* BSPD Status */
   if (getBSPD_Status() == false)
   {
       DEBUG_PRINT("Fault Monitor: BSPD is down!\n");
       DEBUG_PRINT("Fault Monitor: Waiting for BSPD OK.\n");
       DEBUG_PRINT("Fault Monitor: -- help --\n");
       DEBUG_PRINT("Fault Monitor: This is IL_B in the 2019_BMU schematic.\n");
   }

   while (getBSPD_Status() == false)
   {
      vTaskDelay(10);
   }

   DEBUG_PRINT("Fault Monitor: BSPD OK.\n");

   /* HVD Status */
   if (getHVD_Status() == false)
   {
       DEBUG_PRINT("Fault Monitor: HVD is down!\n");
       DEBUG_PRINT("Fault Monitor: Waiting for HVD OK.\n");
       DEBUG_PRINT("Fault Monitor: -- help --\n");
       DEBUG_PRINT("Fault Monitor: This is IL_C in the 2019_BMU schematic.\n");
       DEBUG_PRINT("Fault Monitor: Things to check:\n");
       DEBUG_PRINT("Fault Monitor:  * HVD is plugged in.\n");
       DEBUG_PRINT("Fault Monitor:  * HV connectors are plugged in.\n");
   }

   while (getHVD_Status() == false)
   {
      vTaskDelay(10);
   }

   DEBUG_PRINT("Fault Monitor: HVD OK.\n");

   /* IL Status */

   if (getIL_Status() == false)
   {
       DEBUG_PRINT("Fault Monitor: IL is down!\n");
       DEBUG_PRINT("Fault Monitor: Waiting for IL OK.\n");
       DEBUG_PRINT("Fault Monitor: -- help --\n");
       DEBUG_PRINT("Fault Monitor: This is IL_F in the 2019_BMU schematic.\n");
       DEBUG_PRINT("Fault Monitor: Things to check:\n");
       DEBUG_PRINT("Fault Monitor:  * IMD has not faulted.\n");
       DEBUG_PRINT("Fault Monitor:  * BSPD is in `on` position.\n");
   }

   while (getIL_Status() == false) {
      vTaskDelay(10);
   };

   DEBUG_PRINT("Fault Monitor: IL Started\n");

   /* IL checks complete at this point, fault monitoring system ready */

   fsmSendEvent(&fsmHandle, EV_FaultMonitorReady, portMAX_DELAY);

   if (registerTaskToWatch(FAULT_TASK_ID, 2*pdMS_TO_TICKS(FAULT_MEASURE_TASK_PERIOD), false, NULL) != HAL_OK)
   {
     ERROR_PRINT("Fault Monitor: Failed to register fault monitor task with watchdog!\n");
     Error_Handler();
   }

   while (1)
   {
      if (getHVIL_Status() == false)
      {
         ERROR_PRINT("Fault Monitor: HVIL broken!\n");
         fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);

         while (1) {
            watchdogTaskCheckIn(FAULT_TASK_ID);
            vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
         }
      }

      if (getIL_Status() == false)
      {
         ERROR_PRINT("Fault Monitor: IL broken!\n");
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
