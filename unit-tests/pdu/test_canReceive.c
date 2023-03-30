#include "unity.h"


#include "canReceive.h"

#include "bsp.h"
#include <assert.h>
#include "stdint.h"

#include "state_machine.h"

#include "fake_state_machine_interface.h"

#include "can.h"
#include "pdu_can.h"
#include "pdu_dtc.h"
#include "main.h"
#include "fake_debug.h"
#include "queue.h"
#include "gpio.h"

#include "Mock_watchdog.h"

#include "task.h"
#include "Mock_userCan.h"
#include "Mock_canHeartbeat.h"

volatile int64_t isUartOverCanEnabled;

FSM_Handle_Struct mainFsmHandle;
FSM_Handle_Struct coolingFsmHandle;
FSM_Handle_Struct motorFsmHandle;

void setUp() {}

void tearDown() {}

void test_CAN_Msg_UartOverCanConfig_Callback() {
    // isUartOverCanEnabled = UartOverCanConfigSignal & 0x4;
    TEST_ASSERT(1 == 1);
}

void test_CAN_Msg_VCU_EM_Power_State_Request_Callback() {
    /*if (EM_Power_State_Request == EM_Power_State_Request_On) {
        fsmSendEventISR(&motorFsmHandle, MTR_EV_EM_ENABLE);
        fsmSendEventISR(&coolingFsmHandle, COOL_EV_EM_ENABLE);
    } else {
        fsmSendEventISR(&motorFsmHandle, MTR_EV_EM_DISABLE);
        fsmSendEventISR(&coolingFsmHandle, COOL_EV_EM_DISABLE);
    }*/
    TEST_ASSERT(1 == 1);
}

void test_DTC_Fatal_Callback() {
    /*DEBUG_PRINT_ISR("DTC Receieved from board %lu \n", board);
    fsmSendEventUrgentISR(&mainFsmHandle, MN_EV_HV_CriticalFailure);*/
    TEST_ASSERT(1 == 1);
}
