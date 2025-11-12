//Includes
#include "unity.h"
#include "state_machine.h"

#include "queue.h"

#include <pthread.h>
#include "Mock_debug.h"
#include "fake_debug.h"

#include "main.h"

#include "task.h"

#include "Mock_watchdog.h"

#include "fake_state_machine_interface.h"

#include <unistd.h>

#include "Mock_generalErrorHandler.h"

volatile int64_t StateMachineBoardID;
volatile int64_t StateMachineWatchdogID;
volatile int64_t StateMachineEvent;
volatile int64_t StateMachineState;
volatile uint64_t StateMachineNewState;

// FSM Data 1

typedef enum States_1_t {
    STATE_1_1 = 0,
    STATE_1_2,
    STATE_1_3,
    STATE_1_4,
    STATE_1_ERROR,
    STATE_1_ANY,
} States_1_t;

typedef enum Events_1_t {
    EV_1_1 = 0,
    EV_1_2,
    EV_1_3,
    EV_1_4,
    EV_1_5,
    EV_1_6,
    EV_1_ANY,
} Events_1_t;


// Transitions Functions


uint32_t transition_1_1(uint32_t event)
{
	switch(event){
		case(EV_1_1):
			return STATE_1_2;
		case(EV_1_2):
			return STATE_1_3;
		case(EV_1_3):
			return STATE_1_4;
	}
    return STATE_1_ERROR;
}




// Transitions
Transition_t transitions_1[] = {
    {STATE_1_1,        EV_1_1,         &transition_1_1},
    {STATE_1_2,        EV_1_2,         &transition_1_1},
    {STATE_1_3,        EV_1_3,         &transition_1_1},
    {STATE_1_4,        EV_1_4,         &transition_1_1},
};


// FSM Data 2

typedef enum States_2_t {
    STATE_2_1 = 0,
    STATE_2_2,
    STATE_2_3,
    STATE_2_4,
    STATE_2_ERROR,
    STATE_2_ANY,
} States_2_t;

typedef enum Events_2_t {
    EV_2_1 = 0,
    EV_2_2,
    EV_2_3,
    EV_2_4,
    EV_2_5,
    EV_2_6,
    EV_2_ANY,
} Events_2_t;


// Transitions Functions

uint32_t transition_2_1(uint32_t event)
{
	switch(event){
		case(EV_2_1):
			return STATE_2_2;
		case(EV_2_2):
			return STATE_2_3;
		case(EV_2_3):
			return STATE_2_4;
	}
    return STATE_2_ERROR;
}

uint32_t transition_2_2(uint32_t event){
	switch(event){
		case(EV_2_1):
			return STATE_2_1;
		case(EV_2_2):
			return STATE_2_1;
		case(EV_2_3):
			return STATE_2_1;
	}
	return STATE_2_ERROR;
}
// Transitions
Transition_t transitions_2[] = {
    {STATE_2_1,        EV_2_1,         &transition_2_1},
    {STATE_2_1,        EV_2_2,         &transition_2_2},
    {STATE_2_2,        EV_2_2,         &transition_2_1},
    {STATE_2_3,        EV_2_3,         &transition_2_1},
    {STATE_2_4,        EV_2_4,         &transition_2_1},
};

#define MAX_NUM_THREADS 8
// Active Thread Addresses
pthread_t thread_ids[MAX_NUM_THREADS] = {0};
uint8_t num_threads = 0;

// Unity Standard Functions
void setUp(void)
{
	fake_mock_init_queues();
	fake_mock_init_debug();

	num_threads = 0;
}

void tearDown(void)
{
	for(int thread = 0; thread < num_threads; thread++) {
		end_task(thread_ids[thread]);
	}
}



// Utils Functions
HAL_StatusTypeDef setup_fsm_handle(uint8_t data_num, FSM_Handle_Struct *handle){
	uint32_t state_any;
	uint32_t event_any;
	Transition_t *transitions;
	size_t events_size;
	size_t transitionTableLength;
	
	uint32_t eventQueueLength = 5;
	uint32_t watchdogTaskId = 0;
	
	uint32_t startingState = 0;

	switch(data_num){
		case(1):
			state_any = STATE_1_ANY;
			event_any = EV_1_ANY;
			transitions = transitions_1;
			transitionTableLength = TRANS_COUNT(transitions_1);
			events_size = sizeof(Events_1_t); 
			break;
		case(2):
			state_any = STATE_2_ANY;
			event_any = EV_2_ANY;
			transitions = transitions_2;
			transitionTableLength = TRANS_COUNT(transitions_2);
			events_size = sizeof(Events_2_t);
			break;
		default:
			return HAL_ERROR;
	}

	
	FSM_Init_Struct init;
    init.maxStateNum = state_any;
    init.maxEventNum = event_any;
    init.sizeofEventEnumType = events_size;
    init.ST_ANY = state_any;
    init.EV_ANY = event_any;
    init.transitions = transitions;
    init.transitionTableLength = transitionTableLength;
    init.eventQueueLength = eventQueueLength;
    init.watchdogTaskId = watchdogTaskId;
    if (fsmInit(startingState, &init, handle) != HAL_OK) {
		TEST_MESSAGE("Failure initializing FSM");
		return HAL_ERROR;
    }
    return HAL_OK;


}

#define FSM_START(id) \
	FSM_Handle_Struct fsmHandle_##id; \
	if(HAL_OK != setup_fsm_handle(id, &fsmHandle_##id)){TEST_ASSERT_TRUE(0);} \
	pthread_t fsm_handle_thread_id_##id = fake_mock_fsm_run(&fsmHandle_##id); \
	thread_ids[num_threads] = fsm_handle_thread_id_##id; \
	num_threads++;





/* Ensure FSM initialization works */
void test_Init_Close(void)
{
  	FSM_START(1);
    TEST_ASSERT_TRUE(1);
}

/* Test at least one transition works, STATE_1_1 -> STATE_1_2 */
void test_One_Transition(void)
{
  	FSM_START(1);
	
	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_1);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_1);

	fsmSendEvent(&fsmHandle_1, EV_1_1, 0);
	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_2);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_2);

}


/* Basically just go through FSM 1, STATE_1_1 -> STATE_1_4 */
void test_Multiple_Transition(void)
{
  	FSM_START(1);
	
	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_1);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_1);

	fsmSendEvent(&fsmHandle_1, EV_1_1, 0);
	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_2);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_2);

	fsmSendEvent(&fsmHandle_1, EV_1_2, 0);
	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_3);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_3);
	
	fsmSendEvent(&fsmHandle_1, EV_1_3, 0);
	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_4);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_4);
	TEST_ASSERT_FALSE(fsmGetState(&fsmHandle_1) == STATE_1_3);
	TEST_ASSERT_FALSE(fsmGetState(&fsmHandle_1) == STATE_1_2);
	TEST_ASSERT_FALSE(fsmGetState(&fsmHandle_1) == STATE_1_1);
	
}

// Check that passing an invalid event will make it so the state machine doesn't progress
void test_Fail_Transition(void)
{
	FSM_START(1);
	
	fsmSendEvent(&fsmHandle_1, EV_1_4, 0);
	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_1);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_1);
	
	fsmSendEvent(&fsmHandle_1, EV_1_1, 0);
	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_2);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_2);

	fsmSendEvent(&fsmHandle_1, 97, 0);
	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_2);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_2);
	
	fsmSendEvent(&fsmHandle_1, EV_1_2, 0);
	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_3);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_3);

}

// Check running multiple concurrent FSMs works
void test_Multiple_FSMs(void) {
	FSM_START(1);
	FSM_START(2);

	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_1);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_1);

	fsmSendEvent(&fsmHandle_1, EV_1_1, 0);
	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_2);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_2);

	
	fake_mock_wait_for_fsm_state(&fsmHandle_2, STATE_2_1);
	fsmSendEvent(&fsmHandle_2, EV_2_2, 0);	
	fake_mock_wait_for_fsm_state(&fsmHandle_2, STATE_2_1);

}

// Check sending multiple events in quick succession doesn't affect results
void test_Spam_FSM(void) {
	FSM_START(1);
	fsmSendEvent(&fsmHandle_1, EV_1_1, 0);
	fsmSendEvent(&fsmHandle_1, EV_1_2, 0);
	fsmSendEvent(&fsmHandle_1, EV_1_3, 0);
	fake_mock_wait_for_fsm_state(&fsmHandle_1, STATE_1_4);
	TEST_ASSERT_TRUE(fsmGetState(&fsmHandle_1) == STATE_1_4);
}


