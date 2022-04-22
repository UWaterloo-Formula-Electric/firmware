#include "traction_control.h"
#include "stdint.h"
#include "stdbool.h"
#include "watchdog.h"
#include "debug.h"

#define TRACTION_CONTROL_TASK_ID 3
#define TRACTION_CONTROL_TASK_PERIOD_MS 200
static bool tc_on = false;

void toggle_TC(void)
{
	tc_on = !tc_on;
}
#if 0
static float get_FR_speed()
{
	return 0.0f;
}

static float get_FL_speed()
{
	return 0.0f;
}

static float get_RR_speed()
{
	return 0.0f;
}

static float get_RL_speed()
{
	return 0.0f;
}
#endif

void tractionControlTask(void *pvParameters)
{
	if (registerTaskToWatch(TRACTION_CONTROL_TASK_ID, 2*pdMS_TO_TICKS(TRACTION_CONTROL_TASK_PERIOD_MS), false, NULL) != HAL_OK)
	{
		ERROR_PRINT("ERROR: Failed to init traction control task, suspending traction control task\n");
		while(1);
	}
	while(1)
	{
#if 0
		float FR_speed = 0.0f;
		float FL_speed = 0.0f;
		float RR_speed = 0.0f;
		float RL_speed = 0.0f;
#endif
	}

}
