#include "traction_control.h"

#define TRACTION_CONTROL_TASK_ID 3
#define TRACTION_CONTROL_TASK_PERIOD_MS 200
static bool tc_on = false;

void toggle_TC(void)
{
	tc_on = !tc_on;
}

static float get_FR_speed()
{
}

static float get_FL_speed()
{

}

static float get_RR_speed()
{

}

static float get_RL_speed()
{

}

void tractionControlTask(void *pvParameters)
{
	if (registerTaskToWatch(TRACTION_CONTROL_TASK_ID, 2*pdMS_TO_TICKS(TRACTION_CONTROL_TASK_PERIOD_MS), false, NULL) != HAL_OK)
	{
		ERROR_PRINT("ERROR: Failed to init traction control task, suspending traction control task\n");
		while(1);
	}
	while(1)
	{
		float FR_speed = 
		float FL_speed = 
		float RR_speed = 
		float RL_speed = 
		
	}

}
