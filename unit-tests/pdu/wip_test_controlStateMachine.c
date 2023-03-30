#include "unity.h"

#include "controlStateMachine.h"

#include "Mock_userCan.h"

#include "fake_debug.h"

#include "can.h"
#include "dcu_can.h"
#include "tim.h"
#include "timers.h"
#include "main.h"
#include "queue.h"
#include "gpio.h"
#include "task.h"

#include "state_machine.h"

#include "fake_state_machine_interface.h"

#include "Mock_canReceive.h"


#include "Mock_watchdog.h"
#include "Mock_canHeartbeat.h"



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

}


