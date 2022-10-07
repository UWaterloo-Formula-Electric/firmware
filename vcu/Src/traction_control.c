#include "traction_control.h"
#include "stdint.h"
#include "stdbool.h"
#include "watchdog.h"
#include "debug.h"
#include "motorController.h"
#include "vcu_F7_can.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp.h"
#include "stm32f7xx_hal_tim.h"

#define PI 3.14159

#define TRACTION_CONTROL_TASK_ID 3
#define TRACTION_CONTROL_TASK_PERIOD_MS 200
#define RPM_TO_RADS(rpm) (rpm*2*PI/60.0f)


// For every 1rad/s, decrease torque by kP
#define kP (0.5f)


// With our tire radius, rads/s ~ km/h
#define ERROR_FLOOR_RADS (10.0f)
#define ADJUSTMENT_TORQUE_FLOOR (5.0f)


static bool tc_on = false;

void disable_TC(void)
{
	tc_on = false;
}

void toggle_TC(void)
{
	tc_on = !tc_on;
}
static float get_FR_speed()
{
	//Value comes from WSB
	return SpeedWheelRightFront;
}

static float get_FL_speed()
{
	//Value comes from WSB
	return SpeedWheelLeftFront;
}

static float get_RR_speed()
{
	//Value comes from MC
	return RPM_TO_RADS(SpeedMotorRight);
}

static float get_RL_speed()
{
	//Value comes from MC
	return RPM_TO_RADS(SpeedMotorLeft);
}

void tractionControlTask(void *pvParameters)
{
	if (registerTaskToWatch(TRACTION_CONTROL_TASK_ID, 2*pdMS_TO_TICKS(TRACTION_CONTROL_TASK_PERIOD_MS), false, NULL) != HAL_OK)
	{
		ERROR_PRINT("ERROR: Failed to init traction control task, suspending traction control task\n");
		while(1);
	}

	float torque_max = MAX_TORQUE_DEMAND_DEFAULT;
	float torque_adjustment = ADJUSTMENT_TORQUE_FLOOR;
	float FR_speed = 0.0f; //front right wheel speed
	float FL_speed = 0.0f; //front left wheel speed
	float RR_speed = 0.0f; //rear right wheel speed
	float RL_speed = 0.0f; //rear left wheel speed
	float error_left = 0.0f; //error between left rear and front
	float error_right = 0.0f; //error between right rear and front
	TickType_t xLastWakeTime = xTaskGetTickCount();

	while(1)
	{
		torque_max = MAX_TORQUE_DEMAND_DEFAULT;

		if(tc_on)
		{
			torque_adjustment = ADJUSTMENT_TORQUE_FLOOR;
			FR_speed = get_FR_speed(); 
			FL_speed = get_FL_speed(); 
			RR_speed = get_RR_speed(); 
			RL_speed = get_RL_speed(); 

			error_left = RL_speed - FL_speed;
			error_right = RR_speed - FR_speed;

			//calculate error. This is a P-controller
			if(error_left > ERROR_FLOOR_RADS || error_right > ERROR_FLOOR_RADS)
			{
				if (error_left > error_right)
				{
					torque_adjustment = error_left * kP;
				}
				else
				{
					torque_adjustment = error_right * kP;
				}
			}

			//clamp values
			torque_max = MAX_TORQUE_DEMAND_DEFAULT - torque_adjustment;
			if(torque_max < ADJUSTMENT_TORQUE_FLOOR)
			{
				torque_max = ADJUSTMENT_TORQUE_FLOOR;
			}
			else if(torque_max > MAX_TORQUE_DEMAND_DEFAULT)
			{
				// Whoa error in TC (front wheel is spinning faster than rear)
				torque_max = MAX_TORQUE_DEMAND_DEFAULT;
			}
		}

		setTorqueLimit(torque_max);

		// Always poll at almost exactly PERIOD
        watchdogTaskCheckIn(TRACTION_CONTROL_TASK_ID);
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TRACTION_CONTROL_TASK_PERIOD_MS));
	}

}
