#include "unity.h"


#include "canReceive.h"

#include "bsp.h"
#include <assert.h>
#include "stdint.h"

#include "state_machine.h"

#include "fake_state_machine_interface.h"

#include "can.h"
#include "wsbrl_can.h"
#include "wsbrr_can.h"
#include "wsbrl_dtc.h"
#include "wsbrr_dtc.h"
#include "main.h"
#include "fake_debug.h"
#include "queue.h"
#include "gpio.h"

#include "Mock_watchdog.h"

#include "task.h"
#include "Mock_userCan.h"
#include "Mock_canHeartbeat.h"

volatile int64_t isUartOverCanEnabled;

FSM_Handle_Struct VCUFsmHandle;

void setUp() {}

void tearDown() {}


void test_DTC_Fatal_Callback ()