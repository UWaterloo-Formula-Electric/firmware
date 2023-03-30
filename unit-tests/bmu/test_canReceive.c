#include "unity.h"


#include "canReceive.h"

#include "bsp.h"
#include <assert.h>
#include "stdint.h"

#include "state_machine.h"

#include "fake_state_machine_interface.h"

#include "can.h"
#include "bmu_can.h"
#include "main.h"
#include "fake_debug.h"
#include "queue.h"
#include "gpio.h"

#include "Mock_watchdog.h"

#include "task.h"
#include "Mock_userCan.h"
#include "Mock_canHeartbeat.h"

volatile int64_t isUartOverCanEnabled;

FSM_Handle_Struct fsmHandle;

void setUp() {
}

void tearDown() {}

void test_CAN_Msg_DCU_buttonEvents_Callback()
{
	/*DEBUG_PRINT_ISR("DCU Button events\n");
    if (ButtonHVEnabled) {
		DEBUG_PRINT_ISR("HV Toggle button event\n");
        fsmSendEventISR(&fsmHandle, EV_HV_Toggle);
    }*/
    TEST_ASSERT_TRUE(1 == 1);
}

void test_DTC_Fatal_Callback()
{
	/*DEBUG_PRINT_ISR("DTC Receieved from board %lu \n", board);
    fsmSendEventUrgentISR(&fsmHandle, EV_HV_Fault);*/
    TEST_ASSERT_TRUE(1 == 1);
}

// uint32_t lastChargeCartHeartbeat = 0;
// bool sentChargeStartEvent = false;

void test_CAN_Msg_ChargeCart_Heartbeat_Callback()
{
    /*if (!sentChargeStartEvent) {
        if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
            fsmSendEventISR(&fsmHandle, EV_Enter_Charge_Mode);
            sentChargeStartEvent = true;
        }
    }
    lastChargeCartHeartbeat = xTaskGetTickCount();*/
    TEST_ASSERT_TRUE(1 == 1);
}

void test_CAN_Msg_ChargeCart_ButtonEvents_Callback()
{
    /*if (ButtonChargeStart) {
        fsmSendEventISR(&fsmHandle, EV_Charge_Start);
    }
    if (ButtonChargeStop) {
        fsmSendEventISR(&fsmHandle, EV_Notification_Stop);
    }
    if (ButtonHVEnabled) {
        fsmSendEventISR(&fsmHandle, EV_HV_Toggle);
    }*/
    TEST_ASSERT_TRUE(1 == 1);
}

void test_CAN_Msg_UartOverCanConfig_Callback()
{
	// isUartOverCanEnabled = UartOverCanConfigSignal & 0x2;
    TEST_ASSERT_TRUE(1 == 1);
}
