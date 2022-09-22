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
#define NO_FAULTS (0xFF)
#define HVIL_FAILED_BIT (1)
#define BRB_FAILED_BIT (1 << 1)
#define BSPD_FAILED_BIT (1 << 2)
#define HVD_FAILED_BIT (1 << 3)
#define IL_FAILED_BIT (1 << 4)
#define FSM_STATE_BIT (1 << 5)


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

   BMU_checkFailed = NO_FAULTS; // 00111111

   /* HVIL status */
   HVIL_Control(true);

   if (getHVIL_Status() == false)
   {
       BMU_checkFailed = NO_FAULTS & ~(HVIL_FAILED_BIT);
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
       BMU_checkFailed = NO_FAULTS & ~(BRB_FAILED_BIT);
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
       BMU_checkFailed = NO_FAULTS & ~(BSPD_FAILED_BIT);

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
       BMU_checkFailed = NO_FAULTS & ~(HVD_FAILED_BIT);

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
       BMU_checkFailed = NO_FAULTS & ~(IL_FAILED_BIT);

       DEBUG_PRINT("Fault Monitor: IL is down!\n");
       DEBUG_PRINT("Fault Monitor: Waiting for IL OK.\n");
       DEBUG_PRINT("Fault Monitor: -- help --\n");
       DEBUG_PRINT("Fault Monitor: This is IL_F in the 2019_BMU schematic.\n");
       DEBUG_PRINT("Fault Monitor: Things to check:\n");
       DEBUG_PRINT("Fault Monitor:  * IMD has not faulted.\n");
       DEBUG_PRINT("Fault Monitor:  * TSMS is in `on` position.\n");
   }

   while (getIL_Status() == false) {
      vTaskDelay(10);
   };

   DEBUG_PRINT("Fault Monitor: IL Started\n");

	
   /* Prevents race condition where Fault Monitor passes before system is setup*/
   if (fsmGetState(&fsmHandle) != STATE_Wait_System_Up)
   {
         BMU_checkFailed = NO_FAULTS & ~(FSM_STATE_BIT);

   	   DEBUG_PRINT("Fault Monitor: Waiting for fsm to be in state: STATE_Wait_System_Up\n");
   }
   while (fsmGetState(&fsmHandle) != STATE_Wait_System_Up)
   {
		vTaskDelay(10);
   }
	
   DEBUG_PRINT("Fault Monitor: fsm in proper state: STATE_Wait_System_Up\n");

   /* IL checks complete at this point, fault monitoring system ready */

   fsmSendEvent(&fsmHandle, EV_FaultMonitorReady, portMAX_DELAY);

   if (registerTaskToWatch(FAULT_TASK_ID, 2*pdMS_TO_TICKS(FAULT_MEASURE_TASK_PERIOD), false, NULL) != HAL_OK)
   {
     ERROR_PRINT("Fault Monitor: Failed to register fault monitor task with watchdog!\n");
     Error_Handler();
   }
	
   sendCAN_BMU_Interlock_Loop_Status();

   bool cbrb_pressed = false;
   while (1)
   {
		if (getHVIL_Status() == false)
		{
				ERROR_PRINT("Fault Monitor: HVIL broken!\n");
            BMU_checkFailed = NO_FAULTS & ~(HVIL_FAILED_BIT);

				fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
				while (1) {
					watchdogTaskCheckIn(FAULT_TASK_ID);
					vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
				}
		}

		bool il_ok = getIL_Status();
		bool cbrb_ok = getCBRB_IL_Status();
		if(!cbrb_ok && !cbrb_pressed)
		{	
			ERROR_PRINT("Fault Monitor: Cockpit BRB pressed\n");
         BMU_checkFailed = NO_FAULTS & ~(BRB_FAILED_BIT);

			fsmSendEventUrgent(&fsmHandle, EV_Cockpit_BRB_Pressed, portMAX_DELAY);
			cbrb_pressed = true;
		}
		else if (cbrb_ok && cbrb_pressed)
		{
			fsmSendEvent(&fsmHandle, EV_Cockpit_BRB_Unpressed, portMAX_DELAY);
			cbrb_pressed = false;
		}
		else if (!il_ok && cbrb_ok)
		{
				ERROR_PRINT("Fault Monitor: IL broken!\n");
            BMU_checkFailed = NO_FAULTS & ~(IL_FAILED_BIT);

				fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
				while (1) {
					watchdogTaskCheckIn(FAULT_TASK_ID);
					vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
				}
		}

		watchdogTaskCheckIn(FAULT_TASK_ID);
		vTaskDelay(FAULT_MEASURE_TASK_PERIOD);

      sendCAN_BMU_Interlock_Loop_Status();

   }

#else

   fsmSendEvent(&fsmHandle, EV_FaultMonitorReady, portMAX_DELAY);

   while (1) {
		vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
   }

#endif
}
