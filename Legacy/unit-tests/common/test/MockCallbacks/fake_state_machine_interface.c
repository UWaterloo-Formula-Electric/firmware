#include "fake_state_machine_interface.h"
#include "task.h"
#include "unity.h"
#include <unistd.h>
void *my_test_fsm_task(void *args){
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	fsmTaskFunction(args);
}
pthread_t fake_mock_fsm_run(FSM_Handle_Struct *handle) {
	return start_task(my_test_fsm_task, handle);
}

pthread_t fake_fsmInit(HAL_StatusTypeDef (*init_function)(void), FSM_Handle_Struct *handle) {
	init_function();
	fake_mock_fsm_run(handle);
}


#define POLL_TIME_US (20 * 1000) // 20ms
#define POLL_DURATION_US (1 * 100 * 1000) // 100ms

void fake_mock_wait_for_fsm_state(FSM_Handle_Struct *handle, uint32_t state) {
	
	uint32_t count = 0;
	while(fsmGetState(handle) != state && count < POLL_DURATION_US / POLL_TIME_US){
		usleep(POLL_TIME_US);
		count++;
	}
	if(count >= POLL_DURATION_US / POLL_TIME_US) {
		char error_message[70];
		sprintf(error_message, "Failed Poll waiting on State of value: %d, Actual State: %d", state, fsmGetState(handle));
		TEST_FAIL_MESSAGE(error_message);
	}
}
