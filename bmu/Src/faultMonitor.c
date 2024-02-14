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
#include "bmu_can.h"

#define FAULT_MEASURE_TASK_PERIOD 100
#define FAULT_TASK_ID 6

#define ENABLE_IL_CHECKS

#define HVIL_ENABLED (0)

// CAN Logging
#define HVIL_FAILED_BIT  (1 << 0)
#define BRB_FAILED_BIT   (1 << 1)
#define BSPD_FAILED_BIT  (1 << 2)
#define HVD_FAILED_BIT   (1 << 3)
#define IL_FAILED_BIT    (1 << 4)
#define FSM_STATE_BIT    (1 << 5)
#define CBRB_FAILED_BIT  (1 << 6)

bool skip_il = false;

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
#if HVIL_ENABLED
	return (HAL_GPIO_ReadPin(HVIL_SENSE_GPIO_Port, HVIL_SENSE_Pin) == GPIO_PIN_SET);
#else
	return true;
#endif
}

// IL in to the BMU
bool getIL_BRB_Status()
{
   return (HAL_GPIO_ReadPin(IL_SENSE_GPIO_Port, IL_SENSE_Pin) == GPIO_PIN_SET);
}

// IL in to the BMU
bool getIL_Status()
{
   return (HAL_GPIO_ReadPin(TSMS_SENSE_GPIO_Port, TSMS_SENSE_Pin) == GPIO_PIN_SET || skip_il);
}

// Cockpit BRB IL in to the BMU
bool getCBRB_IL_Status()
{
   return (HAL_GPIO_ReadPin(COCKPIT_BRB_SENSE_GPIO_Port, COCKPIT_BRB_SENSE_Pin) == GPIO_PIN_SET || skip_il);
}

/**
 * Task to continuously monitor the HVIL and IL.
 */
void faultMonitorTask(void *pvParameters)
{

#ifdef ENABLE_IL_CHECKS
   /* IL checks complete at this point, fault monitoring system ready */

   fsmSendEvent(&fsmHandle, EV_FaultMonitorReady, portMAX_DELAY);

   if (registerTaskToWatch(FAULT_TASK_ID, 2*pdMS_TO_TICKS(FAULT_MEASURE_TASK_PERIOD), false, NULL) != HAL_OK)
   {
		ERROR_PRINT("Fault Monitor: Failed to register fault monitor task with watchdog!\n");
		Error_Handler();
   }
	
   bool last_cbrb_ok = false;
   TickType_t xLastWakeTime = xTaskGetTickCount();
   while (1)
   {
		if (getHVIL_Status() == false)
		{
			ERROR_PRINT("Fault Monitor: HVIL broken!\n");
			BMU_checkFailed = HVIL_FAILED_BIT;
			//sendCAN_BMU_Interlock_Loop_Status();

			fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
			while (1) {
				watchdogTaskCheckIn(FAULT_TASK_ID);
				vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
			}
		}

		bool il_ok = getIL_Status();
		bool cbrb_ok = getCBRB_IL_Status();
		bool hvd_ok = getHVD_Status();
		if((!cbrb_ok && hvd_ok) && !last_cbrb_ok)
		{	
			ERROR_PRINT("Fault Monitor: Cockpit BRB pressed\n");
			BMU_checkFailed = CBRB_FAILED_BIT;
			//sendCAN_BMU_Interlock_Loop_Status();

			fsmSendEventUrgent(&fsmHandle, EV_Cockpit_BRB_Pressed, portMAX_DELAY);
			last_cbrb_ok = true;
		}
		else if (cbrb_ok && last_cbrb_ok)
		{
			fsmSendEvent(&fsmHandle, EV_Cockpit_BRB_Unpressed, portMAX_DELAY);
			last_cbrb_ok = false;
		}
		
		if ((!hvd_ok) || (!il_ok && cbrb_ok))
		{
			ERROR_PRINT("Fault Monitor: IL broken!\n");
			BMU_checkFailed = IL_FAILED_BIT;
			//sendCAN_BMU_Interlock_Loop_Status();

			fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
			while (1) {
				watchdogTaskCheckIn(FAULT_TASK_ID);
				vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
			}
		}

		watchdogTaskCheckIn(FAULT_TASK_ID);
		vTaskDelayUntil(&xLastWakeTime, FAULT_MEASURE_TASK_PERIOD);
   }

#else

   fsmSendEvent(&fsmHandle, EV_FaultMonitorReady, portMAX_DELAY);
   while (1) {
		vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
   }

#endif
}
