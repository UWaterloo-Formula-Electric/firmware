    /**
  *****************************************************************************
  * @file    faultMonitor.c
  * @author  Richard Matthews
  * @brief   Module monitoring status of IL
  * @details Contains fault monitor task which monitors the status of the
  * interlock loop (IL) and reports an error when it breaks. Also contains 
  * functions for getting the status of various points along the IL.
  ******************************************************************************
  */

#include "faultMonitor.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "watchdog.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bmu_can.h"
#include "bsp.h"

#define FAULT_MEASURE_TASK_PERIOD 100
#define FAULT_TASK_ID 6

#define ENABLE_IL_CHECKS
#define IL_TEST

// CAN Logging
#define BOTS_FAILED     (1 << BOTS_FAILED_BIT)
#define EBOX_FAILED     (1 << EBOX_FAILED_BIT)
#define BSPD_FAILED     (1 << BSPD_FAILED_BIT)
#define HVD_FAILED      (1 << HVD_FAILED_BIT)
#define AMS_FAILED      (1 << AMS_FAILED_BIT)
#define IMD_FAILED      (1 << IMD_FAILED_BIT)
#define CBRB_FAILED     (1 << CBRB_FAILED_BIT)
#define TSMS_FAILED     (1 << TSMS_FAILED_BIT)
#define HW_CHECK_FAILED (1 << HW_CHECK_FAILED_BIT)
#define FSM_STATE       (1 << FSM_STATE_BIT)

// When charging, some IL checks should be ignored since we are not plugged into vehicle harness
bool skip_il = false;

// IL A
bool getBOTS_Status() {
   return (HAL_GPIO_ReadPin(BOTS_SENSE_GPIO_Port, BOTS_SENSE_Pin) == GPIO_PIN_SET || skip_il);
}

// IL B
// EBOX connectors
bool getEbox_Il_Status() {
   return (HAL_GPIO_ReadPin(IL_EBOX_SENSE_GPIO_Port, IL_EBOX_SENSE_Pin) == GPIO_PIN_SET);
}

// IL C
bool getBSPD_Status() {
   return (HAL_GPIO_ReadPin(BSPD_SENSE_GPIO_Port, BSPD_SENSE_Pin) == GPIO_PIN_SET);
}

// IL D
bool getHVD_Status(){
   return (HAL_GPIO_ReadPin(HVD_SENSE_GPIO_Port, HVD_SENSE_Pin) == GPIO_PIN_SET);
}

// IL E
bool getAMS_Status() {
   return (HAL_GPIO_ReadPin(AMS_SENSE_GPIO_Port, AMS_SENSE_Pin) == GPIO_PIN_SET);
}

// IL F
// This will only read if the IMD has faulted, if the buttons have not been pressed it will return true
bool getIMD_Status() {
   return (HAL_GPIO_ReadPin(IMD_SENSE_GPIO_Port, IMD_SENSE_Pin) == GPIO_PIN_SET);
}

// IL G
bool getCBRB_Status() {
   return (HAL_GPIO_ReadPin(COCKPIT_BRB_SENSE_GPIO_Port, COCKPIT_BRB_SENSE_Pin) == GPIO_PIN_SET || skip_il);
}

// IL H
bool getTSMS_Status() {
   return (HAL_GPIO_ReadPin(TSMS_SENSE_GPIO_Port, TSMS_SENSE_Pin) == GPIO_PIN_SET || skip_il);
}

// IL_I
// power going to contactors 
bool getHwCheck_Status() {
   return (HAL_GPIO_ReadPin(HW_CHECK_SENSE_GPIO_Port, HW_CHECK_SENSE_Pin) == GPIO_PIN_SET);
}

/**
 * Task to continuously monitor the IL.
 */
void faultMonitorTask(void *pvParameters)
{

#ifdef ENABLE_IL_CHECKS

   DEBUG_PRINT("Fault Monitor: IL Started.\n");

   if(getBOTS_Status() == false) {
      BMU_checkFailed = BOTS_FAILED;
      sendCAN_BMU_Interlock_Loop_Status();
      DEBUG_PRINT("Fault Monitor: BOTS is down!\r\n");
      DEBUG_PRINT("Fault Monitor: -- help --\r\n");
      DEBUG_PRINT("Fault Monitor: This is IL_A in 2025 BMU schematic.\r\n");
      DEBUG_PRINT("Fault Monitor: Check BOTS is flipped towards pedal.\r\n");
   }

   while (getBOTS_Status() == false) {
      vTaskDelay(10);
   }

    DEBUG_PRINT("Fault Monitor: BOTS OK.\n");

   if(getEbox_Il_Status() == false) {
      BMU_checkFailed = EBOX_FAILED;
      sendCAN_BMU_Interlock_Loop_Status();
      DEBUG_PRINT("Fault Monitor: Ebox conectors are down!\r\n");
      DEBUG_PRINT("Fault Monitor: -- help --\r\n");
      DEBUG_PRINT("Fault Monitor: This is IL_B in 2025 BMU schematic.\r\n");
      DEBUG_PRINT("Fault Monitor: Check connectors are pushed all the way.\r\n");
   }

   while (getEbox_Il_Status() == false) {
      vTaskDelay(10);
   }

   DEBUG_PRINT("Fault Monitor: EBOX connections OK.\n");

   /* BSPD Status */
   if (getBSPD_Status() == false) {
      BMU_checkFailed = BSPD_FAILED_BIT;
      sendCAN_BMU_Interlock_Loop_Status();

      DEBUG_PRINT("Fault Monitor: BSPD is down!\r\n");
      DEBUG_PRINT("Fault Monitor: Waiting for BSPD OK.\r\n");
      DEBUG_PRINT("Fault Monitor: This is IL_C in the 2025 BMU schematic.\r\n");
      DEBUG_PRINT("Fault Monitor: -- help --\r\n");
      DEBUG_PRINT("Fault Monitor: Make sure reset buttons are pressed\r\n");
   }

   while (getBSPD_Status() == false) {
      vTaskDelay(10);
   }

   DEBUG_PRINT("Fault Monitor: BSPD OK.\n");

   /* HVD Status */
   if (getHVD_Status() == false) {
      BMU_checkFailed = HVD_FAILED_BIT;
      sendCAN_BMU_Interlock_Loop_Status();

      DEBUG_PRINT("Fault Monitor: HVD is down!\r\n");
      DEBUG_PRINT("Fault Monitor: Waiting for HVD OK.\r\n");
      DEBUG_PRINT("Fault Monitor: This is IL D in the 2025 BMU schematic.\r\n");
      DEBUG_PRINT("Fault Monitor: -- help --\r\n");
      DEBUG_PRINT("Fault Monitor: Things to check:\r\n");
      DEBUG_PRINT("Fault Monitor:  * HVD is plugged in.\r\n");
   }

   while (getHVD_Status() == false) {
      vTaskDelay(10);
   }

   DEBUG_PRINT("Fault Monitor: HVD OK.\n");

    #ifdef IL_TEST
    AMS_CONT_CLOSE;
    #endif

   if(getAMS_Status() == false) {
      BMU_checkFailed = AMS_FAILED;
      sendCAN_BMU_Interlock_Loop_Status();
      DEBUG_PRINT("Fault Monitor: AMS is down!\r\n");
      DEBUG_PRINT("Fault Monitor: This is IL E in 2025 BMU schematic.\r\n");
      DEBUG_PRINT("Fault Monitor: -- help --\r\n");
      DEBUG_PRINT("Fault Monitor: Things to check:\r\n");
      DEBUG_PRINT("Fault Monitor: * AMS reset button was pressed\r\n");
      DEBUG_PRINT("Fault Monitor: * Cell voltages and temps\r\n");
   }

   while (getAMS_Status() == false) {
      vTaskDelay(10);
   }

   DEBUG_PRINT("Fault Monitor: AMS OK.\n");

   if(getIMD_Status() == false) {
      BMU_checkFailed = IMD_FAILED;
      sendCAN_BMU_Interlock_Loop_Status();
      DEBUG_PRINT("Fault Monitor: IMD is down!\r\n");
      DEBUG_PRINT("Fault Monitor: This is IL F in 2025 BMU schematic.\r\n");
      DEBUG_PRINT("Fault Monitor: -- help --\r\n");
      DEBUG_PRINT("Fault Monitor: IMD is returning a fault!.\r\n");
      DEBUG_PRINT("Fault Monitor: Things to check:\n");
      DEBUG_PRINT("Fault Monitor: * isolation \r\n");
      DEBUG_PRINT("Fault Monitor: * all IMD connections are secure\r\n");
   }

   while (getIMD_Status() == false) {
      vTaskDelay(10);
   }

   DEBUG_PRINT("Fault Monitor: IMD OK.\n");

   // There is no sense at output of IMD, only checks if IMD has faulted
   // if IMD has not faulted but reset button was not pressed this can return false
   // even if CBRB is not pressed in
   if(getCBRB_Status() == false) {
      BMU_checkFailed = CBRB_FAILED;
      sendCAN_BMU_Interlock_Loop_Status();
      DEBUG_PRINT("Fault Monitor: CBRB is down!\r\n");
      DEBUG_PRINT("Fault Monitor: This is IL G in 2025 BMU schematic.\r\n");
      DEBUG_PRINT("Fault Monitor: Things to check:\n");
      DEBUG_PRINT("Fault Monitor: * CBRB is not pressed in\r\n");
      DEBUG_PRINT("Fault Monitor: * IMD reset button was pressed\r\n");
   }

   while (getCBRB_Status() == false) {
      vTaskDelay(10);
   }

   DEBUG_PRINT("Fault Monitor: CBRB OK.\n");
	
   if(getTSMS_Status() == false) {
      BMU_checkFailed = TSMS_FAILED;
      sendCAN_BMU_Interlock_Loop_Status();
      DEBUG_PRINT("Fault Monitor: TSMS is down!\r\n");
      DEBUG_PRINT("Fault Monitor: This is IL H in 2025 BMU schematic.\r\n");
      DEBUG_PRINT("Fault Monitor: Things to check:\n");
      DEBUG_PRINT("Fault Monitor: * TSMS is switched on\r\n");
   }

   while (getTSMS_Status() == false) {
      vTaskDelay(10);
   }

   DEBUG_PRINT("Fault Monitor: TSMS OK.\n");

   if(getHwCheck_Status() == false) {
      BMU_checkFailed = HW_CHECK_FAILED;
      sendCAN_BMU_Interlock_Loop_Status();
      DEBUG_PRINT("Fault Monitor: HW check is down!\r\n");
      DEBUG_PRINT("Fault Monitor: This is IL I in 2025 BMU schematic.\r\n");
      DEBUG_PRINT("Fault Monitor: This is power going to contactors\r\n");
   }

   while(getHwCheck_Status() == false) {
    vTaskDelay(10);
   }

   DEBUG_PRINT("Fault Monitor: HW check ok\r\n");

   /* Prevents race condition where Fault Monitor passes before system is setup*/
   if (fsmGetState(&fsmHandle) != STATE_Wait_System_Up)
   {
      BMU_checkFailed = FSM_STATE_BIT;
      sendCAN_BMU_Interlock_Loop_Status();
      DEBUG_PRINT("Fault Monitor: Waiting for fsm to be in state: STATE_Wait_System_Up\n");
   }
   while (fsmGetState(&fsmHandle) != STATE_Wait_System_Up)
   {
      DEBUG_PRINT("waiting\r\n");
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
	
   bool last_cbrb_ok = false;
   TickType_t xLastWakeTime = xTaskGetTickCount();
   while (1)
   {
		if (getBOTS_Status() == false) {
			ERROR_PRINT("Fault Monitor: BOTS tripped!\n");
			BMU_checkFailed = BOTS_FAILED;
			sendCAN_BMU_Interlock_Loop_Status();

			fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
			while (1) {
				watchdogTaskCheckIn(FAULT_TASK_ID);
				vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
			}
		}

      if (getEbox_Il_Status() == false) {
			ERROR_PRINT("Fault Monitor: EBOX connector disconnected!\n");
			BMU_checkFailed = EBOX_FAILED;
			sendCAN_BMU_Interlock_Loop_Status();

			fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
			while (1) {
				watchdogTaskCheckIn(FAULT_TASK_ID);
				vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
			}
		}

      if (getBSPD_Status() == false) {
			ERROR_PRINT("Fault Monitor: BSPD tripped!\n");
			BMU_checkFailed = BSPD_FAILED;
			sendCAN_BMU_Interlock_Loop_Status();

			fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
			while (1) {
				watchdogTaskCheckIn(FAULT_TASK_ID);
				vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
			}
		}

      if (getHVD_Status() == false) {
			ERROR_PRINT("Fault Monitor: HVD removed!\n");
			BMU_checkFailed = HVD_FAILED;
			sendCAN_BMU_Interlock_Loop_Status();

			fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
			while (1) {
				watchdogTaskCheckIn(FAULT_TASK_ID);
				vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
			}
		}

      // AMS and IMD monitored by battery task and IMD
      // task respectively, so won't monitor here

		bool il_ok = getHwCheck_Status();
		bool cbrb_ok = getCBRB_Status();
		bool hvd_ok = getHVD_Status();
		if((!cbrb_ok && hvd_ok) && !last_cbrb_ok) {	
			ERROR_PRINT("Fault Monitor: Cockpit BRB pressed\n");
			BMU_checkFailed = CBRB_FAILED_BIT;
			sendCAN_BMU_Interlock_Loop_Status();

			fsmSendEventUrgent(&fsmHandle, EV_Cockpit_BRB_Pressed, portMAX_DELAY);
			last_cbrb_ok = true;
		}
		else if (cbrb_ok && last_cbrb_ok) {
			fsmSendEvent(&fsmHandle, EV_Cockpit_BRB_Unpressed, portMAX_DELAY);
			last_cbrb_ok = false;
		}
		
		if ((!hvd_ok) || (!il_ok && cbrb_ok)) {
			ERROR_PRINT("Fault Monitor: HW check failed!\n");
			BMU_checkFailed = HW_CHECK_FAILED;
			sendCAN_BMU_Interlock_Loop_Status();

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
