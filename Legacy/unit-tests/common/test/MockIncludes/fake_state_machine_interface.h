#ifndef FAKE_STATE_MACHINE_INTERFACE_H
#define FAKE_STATE_MACHINE_INTERFACE_H
#include "task.h"
#include "fake_debug.h"
#include "state_machine.h"



pthread_t fake_mock_fsm_init(HAL_StatusTypeDef (*init_function)(void), FSM_Handle_Struct *handle);
pthread_t fake_mock_fsm_run(FSM_Handle_Struct *handle);
void fake_mock_wait_for_fsm_state(FSM_Handle_Struct *handle, uint32_t state);
#endif
