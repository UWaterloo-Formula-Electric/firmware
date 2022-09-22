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

void toggle_TC(void)
{
	tc_on = !tc_on;
}
/*
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
	TickType_t xLastWakeTime;
	if (registerTaskToWatch(TRACTION_CONTROL_TASK_ID, 2*pdMS_TO_TICKS(TRACTION_CONTROL_TASK_PERIOD_MS), false, NULL) != HAL_OK)
	{
		ERROR_PRINT("ERROR: Failed to init traction control task, suspending traction control task\n");
		while(1);
	}
	while(1)
	{
		xLastWakeTime = xTaskGetTickCount();
		
		float output_torque = MAX_TORQUE_DEMAND_DEFAULT;
		float adjustment_factor = 0.0f;
		//float FR_speed = get_FR_speed();
		float FL_speed = get_FL_speed();
		//float RR_speed = get_RR_speed();
		float RL_speed = get_RL_speed();

		float front_speed = FL_speed; //(FR_speed + FL_speed)/2.0f;
		float rear_speed = RL_speed; //(RR_speed + RL_speed)/2.0f;

		float error = rear_speed - front_speed;
		if(error > ERROR_FLOOR_RADS)
		{
			adjustment_factor = error * kP;
		}

		if(true)
		{
			output_torque = MAX_TORQUE_DEMAND_DEFAULT - adjustment_factor;
			if(output_torque < ADJUSTMENT_TORQUE_FLOOR)
			{
				output_torque = ADJUSTMENT_TORQUE_FLOOR;
			}
			else if(output_torque > MAX_TORQUE_DEMAND_DEFAULT)
			{
				// Whoa error in TC
				output_torque = MAX_TORQUE_DEMAND_DEFAULT;
			}
			setTorqueLimit(output_torque);
		}
		else
		{
			setTorqueLimit(MAX_TORQUE_DEMAND_DEFAULT);
		}
		// Always poll at almost exactly PERIOD
        watchdogTaskCheckIn(TRACTION_CONTROL_TASK_ID);
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TRACTION_CONTROL_TASK_PERIOD_MS));
	}

}
