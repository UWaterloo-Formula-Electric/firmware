#include "unity.h"


#include "canReceive.h"

#include "bsp.h"
#include <assert.h>
#include "stdint.h"

#include "state_machine.h"

#include "fake_state_machine_interface.h"

#include "can.h"
#include "dcu_can.h"
#include "main.h"
#include "fake_debug.h"
#include "queue.h"
#include "gpio.h"

#include "Mock_watchdog.h"

#include "task.h"
#include "Mock_userCan.h"
#include "Mock_canHeartbeat.h"

volatile int64_t isUartOverCanEnabled;

FSM_Handle_Struct DCUFsmHandle;



void setUp() {
}

void tearDown() {}

void test_getHVState()
{
    HV_Power_State = 32;
    TEST_ASSERT_TRUE(getHVState() == (bool)HV_Power_State);
    HV_Power_State = 0;
    TEST_ASSERT_TRUE(getHVState() == (bool)HV_Power_State);
}

void test_getEMState()
{
    EM_State = 30;
    TEST_ASSERT_TRUE(getEMState() == (bool)EM_State);
    EM_State = 0;
    TEST_ASSERT_TRUE(getEMState() == (bool)EM_State);
}

/*
#define IMD_FAIL_LED_ON HAL_GPIO_WritePin(IMD_LED_EN_GPIO_Port, IMD_LED_EN_Pin, GPIO_PIN_SET);
#define AMS_FAIL_LED_ON HAL_GPIO_WritePin(AMS_LED_RED_EN_GPIO_Port, AMS_LED_RED_EN_Pin, GPIO_PIN_SET);
*/

void test_CAN_Msg_BMU_DTC_Callback()
{
    DTC_CODE = 0;
    CAN_Msg_BMU_DTC_Callback(0, 0, 0);

	// BTW don't check GPIO pins in your tests
	// GPIO pins will not change as tests are being run on your own machine
	/*
    FATAL_IMD_Failure = 0;
    assert(HAL_GPIO_ReadPin(IMD_LED_EN_GPIO_Port, IMD_LED_EN_Pin)==GPIO_PIN_SET);

    CRITICAL_CELL_VOLTAGE_LOW = 0;
    assert(HAL_GPIO_ReadPin(AMS_LED_RED_EN_GPIO_Port, AMS_LED_RED_EN_Pin)==GPIO_PIN_SET);
	*/
	
}
