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

#include "FreeRTOS.h"
#include "bmu_can.h"
#include "bsp.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "task.h"
#include "watchdog.h"

#define FAULT_MEASURE_TASK_PERIOD 100
#define FAULT_MEASURE_TASK_INIT_PERIOD 10
#define FAULT_TASK_ID 6

#define ENABLE_IL_CHECKS
#define IL_TEST

// #define CLEAR_FAULTS() (BMU_checkFailed = NO_FAULTS)
// #define LATCH_FAULT(fault) (BMU_checkFailed = fault)
// #define UNLATCH_FAULT(fault) (BMU_checkFailed = NO_FAULTS)

// When charging, some IL checks should be ignored since we are not plugged into vehicle harness
bool skip_il = false;

// IL A
bool getBOTS_Status() {
    return (HAL_GPIO_ReadPin(BOTS_SENSE_GPIO_Port, BOTS_SENSE_Pin) == GPIO_PIN_SET);
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
bool getHVD_Status() {
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

// static bool isInitialized = false;
void faultMonitorSendStatusTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        sendCAN_BMU_Interlock_Loop_Status();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(FAULT_MEASURE_TASK_PERIOD));
        // DEBUG_PRINT("BOTS: %d, EBOX: %d, BSPD: %d, HVD: %d, AMS: %d, IMD: %d, CBRB: %d, TSMS: %d, HW_CHECK: %d\n",
        //             getBOTS_Status(), getEbox_Il_Status(), getBSPD_Status(), getHVD_Status(),
        //             getAMS_Status(), getIMD_Status(), getCBRB_Status(), getTSMS_Status(),
        //             getHwCheck_Status());
        BMU_checkFailed = NO_FAULTS;
        if (getBOTS_Status() == false){
            // DEBUG_PRINT("FIM: BOTS\n");
            BMU_checkFailed = BOTS_FAILED;
            continue;
        }

        if (getEbox_Il_Status() == false){
            // DEBUG_PRINT("FIM: EBOX\n");
            BMU_checkFailed = EBOX_FAILED;
            continue;
        }

        if (getBSPD_Status() == false){
            // DEBUG_PRINT("FIM: BSPD\n");
            BMU_checkFailed = BSPD_FAILED;
            continue;
        }

        if (getHVD_Status() == false){
            // DEBUG_PRINT("FIM: HVD\n");
            BMU_checkFailed = HVD_FAILED;
            continue;
        }

        if (getAMS_Status() == false){
            // DEBUG_PRINT("FIM: AMS\n");
            BMU_checkFailed = AMS_FAILED;
            continue;
        }

        if (getIMD_Status() == false){
            // DEBUG_PRINT("FIM: IMD\n");
            BMU_checkFailed = IMD_FAILED;
            continue;
        }

        if (getCBRB_Status() == false){
            // DEBUG_PRINT("FIM: CBRB\n");
            BMU_checkFailed = CBRB_FAILED;
            continue;
        }

        if (getTSMS_Status() == false){
            // DEBUG_PRINT("FIM: TSMS\n");
            BMU_checkFailed = TSMS_FAILED;
            continue;
        }

        if (getHwCheck_Status() == false){
            // DEBUG_PRINT("FIM: HW_CHECK\n");
            BMU_checkFailed = HW_CHECK_FAILED;
            continue;
        }

        // DEBUG_PRINT("FIM: PASSED\n");

    }
}
/**
 * Task to continuously monitor the IL.
 */
void faultMonitorTask(void *pvParameters) {
#ifdef ENABLE_IL_CHECKS

    DEBUG_PRINT("Fault Monitor: IL Started.\n");
    // CLEAR_FAULTS();

    if (getBOTS_Status() == false) {
        // LATCH_FAULT(BOTS_FAILED);
        DEBUG_PRINT("Fault Monitor: BOTS is down!\r\n");
        DEBUG_PRINT("Fault Monitor: -- help --\r\n");
        DEBUG_PRINT("Fault Monitor: This is IL_A in 2025 BMU schematic.\r\n");
        DEBUG_PRINT("Fault Monitor: Check BOTS is flipped towards pedal.\r\n");
    }

    while (getBOTS_Status() == false) {
        // sendCAN_BMU_Interlock_Loop_Status();
        vTaskDelay(10);
    }

    // UNLATCH_FAULT(BOTS_FAILED);
    DEBUG_PRINT("Fault Monitor: BOTS OK.\n");

    if (getEbox_Il_Status() == false) {
        // LATCH_FAULT(EBOX_FAILED);
        // sendCAN_BMU_Interlock_Loop_Status();
        DEBUG_PRINT("Fault Monitor: Ebox conectors are down!\r\n");
        DEBUG_PRINT("Fault Monitor: -- help --\r\n");
        DEBUG_PRINT("Fault Monitor: This is IL_B in 2025 BMU schematic.\r\n");
        DEBUG_PRINT("Fault Monitor: Check connectors are pushed all the way.\r\n");
    }

    while (getEbox_Il_Status() == false) {
        // sendCAN_BMU_Interlock_Loop_Status();
        vTaskDelay(10);
    }

    // UNLATCH_FAULT(EBOX_FAILED);

    DEBUG_PRINT("Fault Monitor: EBOX connections OK.\n");

    /* BSPD Status */
    if (getBSPD_Status() == false) {
        // LATCH_FAULT(BSPD_FAILED);
        // sendCAN_BMU_Interlock_Loop_Status();

        DEBUG_PRINT("Fault Monitor: BSPD is down!\r\n");
        DEBUG_PRINT("Fault Monitor: Waiting for BSPD OK.\r\n");
        DEBUG_PRINT("Fault Monitor: This is IL_C in the 2025 BMU schematic.\r\n");
        DEBUG_PRINT("Fault Monitor: -- help --\r\n");
        DEBUG_PRINT("Fault Monitor: Make sure reset buttons are pressed\r\n");
    }

    while (getBSPD_Status() == false) {
        // sendCAN_BMU_Interlock_Loop_Status();
        vTaskDelay(10);
    }

    // UNLATCH_FAULT(BSPD_FAILED);

    DEBUG_PRINT("Fault Monitor: BSPD OK.\n");

    /* HVD Status */
    if (getHVD_Status() == false) {
        // LATCH_FAULT(HVD_FAILED);
        // sendCAN_BMU_Interlock_Loop_Status();

        DEBUG_PRINT("Fault Monitor: HVD is down!\r\n");
        DEBUG_PRINT("Fault Monitor: Waiting for HVD OK.\r\n");
        DEBUG_PRINT("Fault Monitor: This is IL D in the 2025 BMU schematic.\r\n");
        DEBUG_PRINT("Fault Monitor: -- help --\r\n");
        DEBUG_PRINT("Fault Monitor: Things to check:\r\n");
        DEBUG_PRINT("Fault Monitor:  * HVD is plugged in.\r\n");
    }

    while (getHVD_Status() == false) {
        // sendCAN_BMU_Interlock_Loop_Status();
        vTaskDelay(10);
    }

    // UNLATCH_FAULT(HVD_FAILED);

    DEBUG_PRINT("Fault Monitor: HVD OK.\n");

#ifdef IL_TEST
    AMS_CONT_CLOSE;
#endif

    if (getAMS_Status() == false) {
        // LATCH_FAULT(AMS_FAILED);
        // sendCAN_BMU_Interlock_Loop_Status();
        DEBUG_PRINT("Fault Monitor: AMS is down!\r\n");
        DEBUG_PRINT("Fault Monitor: This is IL E in 2025 BMU schematic.\r\n");
        DEBUG_PRINT("Fault Monitor: -- help --\r\n");
        DEBUG_PRINT("Fault Monitor: Things to check:\r\n");
        DEBUG_PRINT("Fault Monitor: * AMS reset button was pressed\r\n");
        DEBUG_PRINT("Fault Monitor: * Cell voltages and temps\r\n");
    }

    while (getAMS_Status() == false) {
        // sendCAN_BMU_Interlock_Loop_Status();
        vTaskDelay(10);
    }

    // UNLATCH_FAULT(AMS_FAILED);

    DEBUG_PRINT("Fault Monitor: AMS OK.\n");

    if (getIMD_Status() == false) {
        // LATCH_FAULT(IMD_FAILED);
        // sendCAN_BMU_Interlock_Loop_Status();
        DEBUG_PRINT("Fault Monitor: IMD is down!\r\n");
        DEBUG_PRINT("Fault Monitor: This is IL F in 2025 BMU schematic.\r\n");
        DEBUG_PRINT("Fault Monitor: -- help --\r\n");
        DEBUG_PRINT("Fault Monitor: IMD is returning a fault!.\r\n");
        DEBUG_PRINT("Fault Monitor: Things to check:\n");
        DEBUG_PRINT("Fault Monitor: * isolation \r\n");
        DEBUG_PRINT("Fault Monitor: * all IMD connections are secure\r\n");
    }

    while (getIMD_Status() == false) {
        // sendCAN_BMU_Interlock_Loop_Status();
        vTaskDelay(10);
    }

    // UNLATCH_FAULT(IMD_FAILED);

    DEBUG_PRINT("Fault Monitor: IMD OK.\n");

    // There is no sense at output of IMD, only checks if IMD has faulted
    // if IMD has not faulted but reset button was not pressed this can return false
    // even if CBRB is not pressed in
    if (getCBRB_Status() == false) {
        // LATCH_FAULT(CBRB_FAILED);
        // sendCAN_BMU_Interlock_Loop_Status();
        DEBUG_PRINT("Fault Monitor: CBRB is down!\r\n");
        DEBUG_PRINT("Fault Monitor: This is IL G in 2025 BMU schematic.\r\n");
        DEBUG_PRINT("Fault Monitor: Things to check:\n");
        DEBUG_PRINT("Fault Monitor: * CBRB is not pressed in\r\n");
        DEBUG_PRINT("Fault Monitor: * IMD reset button was pressed\r\n");
    }

    while (getCBRB_Status() == false) {
        // sendCAN_BMU_Interlock_Loop_Status();
        vTaskDelay(10);
    }

    // UNLATCH_FAULT(CBRB_FAILED);

    DEBUG_PRINT("Fault Monitor: CBRB OK.\n");

    if (getTSMS_Status() == false) {
        // LATCH_FAULT(TSMS_FAILED);
        // sendCAN_BMU_Interlock_Loop_Status();
        DEBUG_PRINT("Fault Monitor: TSMS is down!\r\n");
        DEBUG_PRINT("Fault Monitor: This is IL H in 2025 BMU schematic.\r\n");
        DEBUG_PRINT("Fault Monitor: Things to check:\n");
        DEBUG_PRINT("Fault Monitor: * TSMS is switched on\r\n");
    }

    while (getTSMS_Status() == false) {
        // sendCAN_BMU_Interlock_Loop_Status();
        vTaskDelay(10);
    }

    // UNLATCH_FAULT(TSMS_FAILED);

    DEBUG_PRINT("Fault Monitor: TSMS OK.\n");

    if (getHwCheck_Status() == false) {
        // LATCH_FAULT(HW_CHECK_FAILED);
        // sendCAN_BMU_Interlock_Loop_Status();
        DEBUG_PRINT("Fault Monitor: HW check is down!\r\n");
        DEBUG_PRINT("Fault Monitor: This is IL I in 2025 BMU schematic.\r\n");
        DEBUG_PRINT("Fault Monitor: This is power going to contactors\r\n");
    }

    while (getHwCheck_Status() == false) {
        // sendCAN_BMU_Interlock_Loop_Status();
        vTaskDelay(10);
    }

    // UNLATCH_FAULT(HW_CHECK_FAILED);

    DEBUG_PRINT("Fault Monitor: HW check ok\r\n");

    /* Prevents race condition where Fault Monitor passes before system is setup*/
    if (fsmGetState(&fsmHandle) != STATE_Wait_System_Up) {
        // LATCH_FAULT(FSM_STATE_FAILED);

        DEBUG_PRINT("Fault Monitor: Waiting for fsm to be in state: STATE_Wait_System_Up\n");
    }
    while (fsmGetState(&fsmHandle) != STATE_Wait_System_Up) {
        // sendCAN_BMU_Interlock_Loop_Status();
        DEBUG_PRINT("waiting\r\n");
        vTaskDelay(10);
    }

    // UNLATCH_FAULT(FSM_STATE_FAILED);

    DEBUG_PRINT("Fault Monitor: fsm in proper state: STATE_Wait_System_Up\n");
    // isInitialized = true;
    /* IL checks complete at this point, fault monitoring system ready */

    fsmSendEvent(&fsmHandle, EV_FaultMonitorReady, portMAX_DELAY);

    if (registerTaskToWatch(FAULT_TASK_ID, 2 * pdMS_TO_TICKS(FAULT_MEASURE_TASK_PERIOD), false, NULL) != HAL_OK) {
        ERROR_PRINT("Fault Monitor: Failed to register fault monitor task with watchdog!\n");
        Error_Handler();
    }

    bool last_cbrb_ok = false;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // CLEAR_FAULTS();
    while (1) {
        if (getBOTS_Status() == false) {
            ERROR_PRINT("Fault Monitor: BOTS tripped!\n");
            // LATCH_FAULT(BOTS_FAILED);
            // sendCAN_BMU_Interlock_Loop_Status();

            fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
            while (1) {
                // sendCAN_BMU_Interlock_Loop_Status();
                watchdogTaskCheckIn(FAULT_TASK_ID);
                vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
            }
        } 
        // else {
        //     UNLATCH_FAULT(BOTS_FAILED);
        // }

        if (getEbox_Il_Status() == false) {
            ERROR_PRINT("Fault Monitor: EBOX connector disconnected!\n");
            // LATCH_FAULT(EBOX_FAILED);
            // sendCAN_BMU_Interlock_Loop_Status();

            fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
            while (1) {
                // sendCAN_BMU_Interlock_Loop_Status();
                watchdogTaskCheckIn(FAULT_TASK_ID);
                vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
            }
        } 
        // else {
        //     UNLATCH_FAULT(EBOX_FAILED);
        // }

        if (getBSPD_Status() == false) {
            ERROR_PRINT("Fault Monitor: BSPD tripped!\n");
            // LATCH_FAULT(BSPD_FAILED);
            // sendCAN_BMU_Interlock_Loop_Status();

            fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
            while (1) {
                // sendCAN_BMU_Interlock_Loop_Status();
                watchdogTaskCheckIn(FAULT_TASK_ID);
                vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
            }
        } 
        // else {
        //     UNLATCH_FAULT(BSPD_FAILED);
        // }

        if (getHVD_Status() == false) {
            ERROR_PRINT("Fault Monitor: HVD removed!\n");
            // LATCH_FAULT(HVD_FAILED);
            // sendCAN_BMU_Interlock_Loop_Status();

            fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
            while (1) {
                // sendCAN_BMU_Interlock_Loop_Status();
                watchdogTaskCheckIn(FAULT_TASK_ID);
                vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
            }
        } 
        // else {
        //     UNLATCH_FAULT(HVD_FAILED);
        // }

        // AMS and IMD monitored by battery task and IMD
        // task respectively, so won't monitor here

        bool il_ok = getHwCheck_Status();
        bool cbrb_ok = getCBRB_Status();
        bool hvd_ok = getHVD_Status();
        if ((!cbrb_ok && hvd_ok) && !last_cbrb_ok) {
            ERROR_PRINT("Fault Monitor: Cockpit BRB pressed\n");
            // LATCH_FAULT(CBRB_FAILED);
            // sendCAN_BMU_Interlock_Loop_Status();

            fsmSendEventUrgent(&fsmHandle, EV_Cockpit_BRB_Pressed, portMAX_DELAY);
            last_cbrb_ok = true;
        } else if (cbrb_ok && last_cbrb_ok) {
            // UNLATCH_FAULT(CBRB_FAILED);
            fsmSendEvent(&fsmHandle, EV_Cockpit_BRB_Unpressed, portMAX_DELAY);
            last_cbrb_ok = false;
        }

        if (getTSMS_Status() == false) {
            ERROR_PRINT("Fault Monitor: TSMS removed!\n");
            // LATCH_FAULT(TSMS_FAILED);
            // sendCAN_BMU_Interlock_Loop_Status();

            fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
            while (1) {
                // sendCAN_BMU_Interlock_Loop_Status();
                watchdogTaskCheckIn(FAULT_TASK_ID);
                vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
            }
        } 
        // else {
        //     UNLATCH_FAULT(TSMS_FAILED);
        // }

        if ((!hvd_ok) || (!il_ok && cbrb_ok)) {
            ERROR_PRINT("Fault Monitor: HW check failed!\n");
            // LATCH_FAULT(HW_CHECK_FAILED);
            // sendCAN_BMU_Interlock_Loop_Status();

            fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, portMAX_DELAY);
            while (1) {
                // sendCAN_BMU_Interlock_Loop_Status();
                watchdogTaskCheckIn(FAULT_TASK_ID);
                vTaskDelay(FAULT_MEASURE_TASK_PERIOD);
            }
        } 
        // else {
        //     UNLATCH_FAULT(HW_CHECK_FAILED);
        // }

        // sendCAN_BMU_Interlock_Loop_Status();
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
