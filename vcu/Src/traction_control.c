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

/*
The motor controllers will return a 16 bit unsigned integer that needs to be converted to an integer value with the middle being at 32768. Negative numbers mean the wheels are spinning backwards, Positive values indicate forward spin
This exists and isn't done in the DBC bc the CAN driver has issues with the order of casting gives us large numbers around the middle point when the speed is around 0 
We want to do (((int32_t)rpm) - 32768)  where the driver will do  (int32_t)((uint32_t)rpm-32768)
*/
#define MC_ENCODER_OFFSET 32768

#define TRACTION_CONTROL_TASK_ID 3
#define TRACTION_CONTROL_TASK_PERIOD_MS 10
#define RPM_TO_RADS(rpm) ((rpm)*2*PI/60.0f)

// Macros for converting RPM to KPH
#define GEAR_RATIO 15.0/52.0
#define M_TO_KM 1.0/1000.0f
#define WHEEL_DIAMETER_M 0.52
#define WHEEL_CIRCUMFERENCE WHEEL_DIAMETER_M*PI
#define HOUR_TO_MIN 60
#define RPM_TO_KPH(rpm) ((rpm)*HOUR_TO_MIN*WHEEL_CIRCUMFERENCE*M_TO_KM*GEAR_RATIO)

// For every 1rad/s, decrease torque by kP
#define TC_kP_DEFAULT (0.1f)
#define TC_MIN_PERCENT_ERROR (0.05f)

// With our tire radius, rads/s ~ km/h
#define TC_ABS_ERROR_FLOOR_RAD_S_DEFAULT (5.0f)
#define TC_TORQUE_MAX_FLOOR_DEFAULT (2.0f)

static bool tc_on = false;

void disable_TC(void)
{
	tc_on = false;
}

void toggle_TC(void)
{
	tc_on = !tc_on;
}

static float get_wheel_speed_FR_RAD_S()
{
	//Value comes from WSB
	return FR_Speed_RAD_S;
}

static float get_wheel_speed_FL_RAD_S()
{
	//Value comes from WSB
	return FL_Speed_RAD_S;
}

static float get_wheel_speed_RR_RAD_S()
{
	//Value comes from MC
	int64_t val = SpeedMotorRight;
	return RPM_TO_RADS(val - MC_ENCODER_OFFSET);
}

static float get_wheel_speed_RL_RAD_S()
{
	//Value comes from MC
	int64_t val = SpeedMotorLeft;
	return RPM_TO_RADS(val - MC_ENCODER_OFFSET);
}

float tc_kP = TC_kP_DEFAULT;
float tc_error_floor_rad_s = TC_ABS_ERROR_FLOOR_RAD_S_DEFAULT;
float tc_min_percent_error = TC_MIN_PERCENT_ERROR;
float tc_torque_max_floor = TC_TORQUE_MAX_FLOOR_DEFAULT;

void tractionControlTask(void *pvParameters)
{
	if (registerTaskToWatch(TRACTION_CONTROL_TASK_ID, 2*pdMS_TO_TICKS(TRACTION_CONTROL_TASK_PERIOD_MS), false, NULL) != HAL_OK)
	{
		ERROR_PRINT("ERROR: Failed to init traction control task, suspending traction control task\n");
		while(1);
	}

	float torque_max = MAX_TORQUE_DEMAND_DEFAULT;
	float torque_adjustment = 0;
	float wheel_speed_FR_RAD_S = 0.0f; //front right wheel speed
	float wheel_speed_FL_RAD_S = 0.0f; //front left wheel speed
	float wheel_speed_RR_RAD_S = 0.0f; //rear right wheel speed
	float wheel_speed_RL_RAD_S = 0.0f; //rear left wheel speed
	TickType_t xLastWakeTime = xTaskGetTickCount();

	//initialized variables so that speed is 0 on startup 
	wheel_speed_RR_RAD_S = MC_ENCODER_OFFSET;
	wheel_speed_RL_RAD_S = MC_ENCODER_OFFSET; 

	while(1)
	{
		torque_max = MAX_TORQUE_DEMAND_DEFAULT;

		wheel_speed_FR_RAD_S = get_wheel_speed_FR_RAD_S(); 
		wheel_speed_FL_RAD_S = get_wheel_speed_FL_RAD_S(); 
		wheel_speed_RR_RAD_S = get_wheel_speed_RR_RAD_S(); 
		wheel_speed_RL_RAD_S = get_wheel_speed_RL_RAD_S(); 

		WheelSpeedRR_RAD_S = wheel_speed_RR_RAD_S;
		WheelSpeedRL_RAD_S = wheel_speed_RL_RAD_S;
		sendCAN_TractionControl_RearWheelSpeed();

		SpeedMotorRightKPH = RPM_TO_KPH(((int64_t)SpeedMotorRight) - MC_ENCODER_OFFSET);
		SpeedMotorLeftKPH = RPM_TO_KPH(((int64_t)SpeedMotorLeft) - MC_ENCODER_OFFSET);
		sendCAN_SpeedMotorKPH();

		if(tc_on)
		{
			torque_adjustment = 0.0f;
			const float wheel_speed_abs_error_left = (wheel_speed_RL_RAD_S - wheel_speed_FL_RAD_S);
			const float wheel_speed_abs_error_right = (wheel_speed_RR_RAD_S - wheel_speed_FR_RAD_S);
			const float wheel_speed_percent_error_left = (wheel_speed_abs_error_left / wheel_speed_RL_RAD_S);
			const float wheel_speed_percent_error_right = (wheel_speed_abs_error_right / wheel_speed_RR_RAD_S);
			const bool tc_active_left = ((wheel_speed_abs_error_left > tc_error_floor_rad_s) && (wheel_speed_percent_error_left > tc_min_percent_error));
			const bool tc_active_right = ((wheel_speed_abs_error_right > tc_error_floor_rad_s) && (wheel_speed_percent_error_right > tc_min_percent_error));
			const float desired_torque_adjustment_left = (wheel_speed_abs_error_left - tc_error_floor_rad_s) * tc_kP;
			const float desired_torque_adjustment_right = (wheel_speed_abs_error_right - tc_error_floor_rad_s) * tc_kP;


			//calculate error. This is a P-controller
			if(tc_active_left || tc_active_right)
			{
				if (wheel_speed_abs_error_left > wheel_speed_abs_error_right)
				{
					torque_adjustment = desired_torque_adjustment_left;
				}
				else
				{
					torque_adjustment = desired_torque_adjustment_right;
				}
			}

			//clamp values
			torque_max = MAX_TORQUE_DEMAND_DEFAULT - torque_adjustment;
			if(torque_max < tc_torque_max_floor)
			{
				torque_max = tc_torque_max_floor;
			}
			else if(torque_max > MAX_TORQUE_DEMAND_DEFAULT)
			{
				// Whoa error in TC (front wheel is spinning faster than rear)
				torque_max = MAX_TORQUE_DEMAND_DEFAULT;
			}

			Torque_Adjustment_Left = (uint32_t) desired_torque_adjustment_right;
			Torque_Adjustment_Right = (uint32_t) desired_torque_adjustment_left;
			sendCAN_TractionControl_TorqueAdjustment();

			TC_AbsErrorLeft_RAD_S = (uint16_t) wheel_speed_abs_error_left;
			TC_AbsErrorRight_RAD_S = (uint16_t) wheel_speed_abs_error_right;
			TC_SlipPercentLeft = (uint8_t) wheel_speed_percent_error_left;
			TC_SlipPercentRight = (uint8_t) wheel_speed_percent_error_right;
			Torque_Max = (uint8_t)torque_max;
			sendCAN_TractionControl_Debug();
		}

		setTorqueLimit(torque_max);

		// Always poll at almost exactly PERIOD
        watchdogTaskCheckIn(TRACTION_CONTROL_TASK_ID);
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TRACTION_CONTROL_TASK_PERIOD_MS));
	}

}
