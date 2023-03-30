#include "unity.h"

#include "controlStateMachine.h"
#include "stm32f7xx_hal.h"
#include "stdbool.h"
#include "FreeRTOS.h"

#include "Mock_userCan.h"

#include "fake_debug.h"

#include "gpio_ports.h"

#include "can.h"
#include "bmu_dtc.h"
#include "bmu_can.h"
#include "tim.h"
#include "timers.h"
#include "main.h"
#include "queue.h"
#include "task.h"
#include "bsp.h"
#include "prechargeDischarge.h"
#include "batteries.h"
#include "chargerControl.h"

#include "cmsis_os.h"
#include "state_machine.h"

#include "fake_state_machine_interface.h"

#include "Mock_canReceive.h"

void setUp(void)
{
	fake_mock_init_queues();
	fake_mock_init_debug();
}

void tearDown(void)
{
}

void test_simple_fsm(void)
{
/*	
	TEST_ASSERT_TRUE(dcuFsmInit() == HAL_OK);
	pthread_t fsm_id = fake_mock_fsm_run(&DCUFsmHandle);
	
	fsmSendEvent(&DCUFsmHandle, EV_HV_Toggle, 0);
	fake_mock_wait_for_fsm_state(&DCUFsmHandle, STATE_HV_Toggle);
	TEST_ASSERT_TRUE(ButtonHVEnabled == 1);



	end_task(fsm_id);
	*/
}


