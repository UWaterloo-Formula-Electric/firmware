#include "unity.h"

#include "canReceive.h"
#include "CRC_CALC.h"

#include "bsp.h"
#include <assert.h>
#include "stdint.h"

#include "state_machine.h"

#include "fake_state_machine_interface.h"

#include "can.h"
#include "vcu_F7_can.h"
#include "vcu_F7_dtc.h"
#include "FreeRTOS.h"

#include "main.h"
#include "fake_debug.h"
#include "queue.h"
#include "gpio.h"

#include "Mock_watchdog.h"

#include "task.h"
#include "Mock_userCan.h"
#include "Mock_canHeartbeat.h"

#include "cmsis_os.h"

#include "motorController.h"
#include "endurance_mode.h"
#include "traction_control.h"

volatile int64_t isUartOverCanEnabled;

FSM_Handle_Struct fsmHandle;

void setUp() {}

void tearDown() {}

/*
 * External Board Statuses:
 * Variables for keeping track of external board statuses that get updated by
 * Can messages
 */
// bool motorControllersStatus = false;
// uint32_t lastBrakeValReceiveTimeTicks = 0;
/*
 * Functions to get external board status
 */
bool test_getHvEnableState()
{
    // return HV_Power_State == HV_Power_State_On;
    TEST_ASSERT(1 == 1);
}

bool test_getMotorControllersStatus()
{
    // return motorControllersStatus;
    TEST_ASSERT(1 == 1);
}

// extern osThreadId driveByWireHandle;

void test_CAN_Msg_DCU_buttonEvents_Callback()
{
    /*DEBUG_PRINT_ISR("Received DCU button Event\n");
    if (ButtonEMEnabled) {
        fsmSendEventISR(&fsmHandle, EV_EM_Toggle);
    }
    else if(ButtonEnduranceToggleEnabled) 
    {
		toggle_endurance_mode();
	}
	else if(ButtonEnduranceLapEnabled)
	{
		trigger_lap();
	}
	else if(ButtonTCEnabled)
	{
		toggle_TC();
	}*/
    TEST_ASSERT(1 == 1);
}

void test_CAN_Msg_PDU_ChannelStatus_Callback()
{
    /*BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (!motorControllersStatus && StatusPowerMCLeft == StatusPowerMCLeft_CHANNEL_ON &&
        StatusPowerMCRight == StatusPowerMCRight_CHANNEL_ON) {
        xTaskNotifyFromISR( driveByWireHandle,
                            (1<<NTFY_MCs_ON),
                            eSetBits,
                            &xHigherPriorityTaskWoken );
        motorControllersStatus = true;
    } else if (motorControllersStatus) {
        // Only send a notification if MCs turned off if MCs were already ON
        xTaskNotifyFromISR( driveByWireHandle,
                            (1<<NTFY_MCs_OFF),
                            eSetBits,
                            &xHigherPriorityTaskWoken );
        motorControllersStatus = false;
    }

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );*/
    TEST_ASSERT(1 == 1);
}

void test_DTC_Fatal_Callback()
{
    // DEBUG_PRINT_ISR("DTC Receieved from board %lu \n", board);
    // fsmSendEventUrgentISR(&fsmHandle, EV_Fatal);
    TEST_ASSERT(1 == 1);
}

void test_CAN_Msg_BMU_HV_Power_State_Callback() {
    /*DEBUG_PRINT_ISR("Receive hv power state\n");
    if (HV_Power_State != HV_Power_State_On) {
        fsmSendEventISR(&fsmHandle, EV_Hv_Disable);
    }*/
}

void test_CAN_Msg_BMU_BrakePedalValue_Callback()
{
    // lastBrakeValReceiveTimeTicks = xTaskGetTickCount();
    TEST_ASSERT(1 == 1);
}

void test_CAN_Msg_BMU_DTC_Callback() {
    /*switch (DTC_CODE) {
        case WARNING_CONTACTOR_OPEN_IMPENDING:
            fsmSendEventISR(&fsmHandle, EV_Hv_Disable);
            break;
        default:
            // Do nothing, other events handled by fatal callback
            break;
    }*/
    TEST_ASSERT(1 == 1);
}

void test_CAN_Msg_TempInverterLeft_Callback() {
    /*static uint32_t lastLeftInverterDTC = 0;
	if (pdMS_TO_TICKS(xTaskGetTickCountFromISR() - lastLeftInverterDTC) <= 2500)
    {
		return;
	}
	if (StateInverterLeft == 0x25)
    {
		sendDTC_WARNING_MOTOR_CONTROLLERS_FAULT_OFF(1);
	    lastLeftInverterDTC = xTaskGetTickCountFromISR();
	}*/
    TEST_ASSERT(1 == 1);
}

void test_CAN_Msg_TempInverterRight_Callback() {
    /*static uint32_t lastRightInverterDTC = 0;
	if (pdMS_TO_TICKS(xTaskGetTickCountFromISR() - lastRightInverterDTC) <= 2500)
    {
		return;
	}
	if (StateInverterRight == 0x25)
    {
		sendDTC_WARNING_MOTOR_CONTROLLERS_FAULT_OFF(0);
	    lastRightInverterDTC = xTaskGetTickCountFromISR();
	}*/
    TEST_ASSERT(1 == 1);
}


void test_CAN_Msg_UartOverCanConfig_Callback()
{
    // isUartOverCanEnabled = UartOverCanConfigSignal & 0x1;
    TEST_ASSERT(1 == 1);
}

void test_CAN_Msg_PDU_DTC_Callback() {
    /*switch (DTC_CODE)
    {
        case ERROR_DCDC_Shutoff:
            //The DCDC unexpectedly stopped working. The PDU turned off cooling and the motors, now disable EM
            if (fsmGetState(&fsmHandle) == STATE_EM_Enable)
            {
                fsmSendEventISR(&fsmHandle, EV_EM_Toggle);
            }
            break;
        default:
            // Do nothing, other events handled by fatal callback
            break;
    }*/
    TEST_ASSERT(1 == 1);
}
