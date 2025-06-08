/**
 *******************************************************************************
 * @file    traction_control.c
 * @author	Unknown
 * @date    Dec 2024
 * @brief   Traction control algorithm (not currently used)
 *
 ******************************************************************************
 */

#include "traction_control.h"
#include "stdint.h"
#include "stdbool.h"
#include "watchdog.h"
#include "debug.h"
#include "motorController.h"
#include "vcu_F7_can.h"
#include "math.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp.h"
#include "stm32f7xx_hal_tim.h"
#include "wheelConstants.h"

/*
The motor controllers will return a 16 bit unsigned integer that needs to be converted to an integer value with the middle being at 32768. Negative numbers mean the wheels are spinning backwards, Positive values indicate forward spin
This exists and isn't done in the DBC bc the CAN driver has issues with the order of casting gives us large numbers around the middle point when the speed is around 0 
We want to do (((int32_t)rpm) - 32768)  where the driver will do  (int32_t)((uint32_t)rpm-32768)
*/
#define MC_ENCODER_OFFSET 32768

#define TRACTION_CONTROL_TASK_ID 3
#define TRACTION_CONTROL_TASK_PERIOD_MS 35
#define TRACTION_CONTROL_TASK_PERIOD_S (((float)TRACTION_CONTROL_TASK_PERIOD_MS)/1000.0f)

#define TC_kP_DEFAULT (10.0f)
#define TC_kI_DEFAULT (0.0f)
#define TC_kD_DEFAULT (0.0f)

// With our tire radius, rads/s ~ km/h
#define SLIP_PERCENT_DEFAULT (0.1f)
#define ADJUSTMENT_TORQUE_FLOOR_DEFAULT (0.0f)
#define ZERO_SPEED_LOWER_BOUND (10.0f)
#define MAX_SLIP (80.0f)
#define INTEGRAL_RESET_SPEED (5.0f)

typedef struct {
	float FL;
	float FR;
	float RL;
	float RR;
} WheelSpeed_S;

typedef struct {
	float torque_max;
	float left_slip;
	float right_slip;
	float torque_adjustment;
	float cum_error;
	float last_error;
} TCData_S;


static bool tc_on = false;

float tc_kP = TC_kP_DEFAULT;
float tc_kI = TC_kI_DEFAULT;
float tc_kD = TC_kD_DEFAULT;
float desired_slip = SLIP_PERCENT_DEFAULT;
float adjustment_torque_floor = ADJUSTMENT_TORQUE_FLOOR_DEFAULT;

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
	return FR_Speed_RAD_S;
}

static float get_FL_speed()
{
	//Value comes from WSB
	return FL_Speed_RAD_S;
}

static float get_RR_speed()
{
	//Value comes from MC
	return INV_Motor_Speed * GEAR_RATIO; // in rpm
}

static float get_RL_speed()
{
	//Value comes from MC
	return INV_Motor_Speed * GEAR_RATIO; // in rpm
}

static void publish_can_data(WheelSpeed_S* wheel_data, TCData_S* tc_data)
{	
	if(NULL == wheel_data || NULL == tc_data)
	{
		ERROR_PRINT("Null pointer passed to publish_can_data\n");
		handleError();
	}
	VCU_wheelSpeed_RL = wheel_data->RL;
	VCU_wheelSpeed_RR = wheel_data->RR;
	sendCAN_RearWheelSpeedRADS();
	vTaskDelay(2); // Added to prevent CAN mailbox full

	FLSpeedKPH = RADS_TO_KPH(wheel_data->FL);
	FRSpeedKPH = RADS_TO_KPH(wheel_data->FR);
	RLSpeedKPH = RADS_TO_KPH(wheel_data->RL);
	RRSpeedKPH = RADS_TO_KPH(wheel_data->RR);
	sendCAN_WheelSpeedKPH();
	vTaskDelay(2); // Added to prevent CAN mailbox full

	TCTorqueMax = tc_data->torque_max;
	TCTorqueAdjustment = tc_data->torque_adjustment;
	TCLeftSlip = tc_data->left_slip;
	TCRightSlip = tc_data->right_slip;
	sendCAN_TractionControlData();
}

static float abs_clamp(float input, float high, float low)
{
	if(input > high)
	{
		return high;
	}
	else if(input < low)
	{
		return low;
	}
	else
	{
		return input;
	}
}

static float compute_side_slip(float front, float rear)
{
	float slip = MAX_SLIP;
	if(fabs(front) < 0.1)
	{
		front = 0.1;
	}
	slip = (rear - front) / front;

	// Clamp error to +/-MAX_SLIP
	slip = abs_clamp(slip, MAX_SLIP, -MAX_SLIP);
	return slip;
}

static float compute_gains(TCData_S* tc_data)
{
	//calculate error. This is a P-controller
	float slip = fmax(tc_data->left_slip, tc_data->right_slip);
	float error = slip - desired_slip;
	tc_data->cum_error += error;
	float de_dt = (error - tc_data->last_error)/(TRACTION_CONTROL_TASK_PERIOD_S);
	tc_data->last_error = error;
	if(error > 0.0)
	{	
		return (tc_kP * error) + (tc_kI * tc_data->cum_error * TRACTION_CONTROL_TASK_PERIOD_S) + (tc_kD * de_dt);
	}

	return 0.0f;
}

static float tc_compute_limit(WheelSpeed_S* wheel_data, TCData_S* tc_data)
{
	if(NULL == wheel_data || NULL == tc_data)
	{
		ERROR_PRINT("Null pointer passed to tc_compute_limit\n");
		handleError();
	}

	tc_data->torque_max = MAX_TORQUE_DEMAND_DEFAULT_NM;
	tc_data->torque_adjustment = 0.0f;

	tc_data->left_slip = compute_side_slip(wheel_data->FL, wheel_data->RL); 
	tc_data->right_slip = compute_side_slip(wheel_data->FR, wheel_data->RR);

	if(fabs(wheel_data->RL) < INTEGRAL_RESET_SPEED && fabs(wheel_data->RR) < INTEGRAL_RESET_SPEED)
	{
		tc_data->cum_error = 0.0f;
	}
	tc_data->torque_adjustment = compute_gains(tc_data);

	float desired_torque = MAX_TORQUE_DEMAND_DEFAULT_NM - tc_data->torque_adjustment;
	tc_data->torque_max = abs_clamp(desired_torque, MAX_TORQUE_DEMAND_DEFAULT_NM, adjustment_torque_floor);
	return tc_data->torque_max;
}



void tractionControlTask(void *pvParameters)
{
	if (registerTaskToWatch(TRACTION_CONTROL_TASK_ID, 2*pdMS_TO_TICKS(TRACTION_CONTROL_TASK_PERIOD_MS), false, NULL) != HAL_OK)
	{
		ERROR_PRINT("ERROR: Failed to init traction control task, suspending traction control task\n");
		Error_Handler();
	}

	WheelSpeed_S wheel_data = {0};
	TCData_S tc_data = {0};

	tc_data.torque_max = MAX_TORQUE_DEMAND_DEFAULT_NM;
	tc_data.torque_adjustment = adjustment_torque_floor;
	tc_data.left_slip = 0.0f; 
	tc_data.right_slip = 0.0f; 
	TickType_t xLastWakeTime = xTaskGetTickCount();

	while(1)
	{
		// rpm
		wheel_data.FL = get_FL_speed(); 
		wheel_data.FR = get_FR_speed(); 
		wheel_data.RL = get_RL_speed(); 
		wheel_data.RR = get_RR_speed(); 
	
		float tc_torque = tc_compute_limit(&wheel_data, &tc_data);
		float output_torque = MAX_TORQUE_DEMAND_DEFAULT_NM;
		if(tc_on && fmax(wheel_data.RL, wheel_data.RR) > ZERO_SPEED_LOWER_BOUND)
		{
			output_torque = tc_torque;
		}
		setTorqueLimit(output_torque);

		publish_can_data(&wheel_data, &tc_data);
		// Always poll at almost exactly PERIOD
        watchdogTaskCheckIn(TRACTION_CONTROL_TASK_ID);
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TRACTION_CONTROL_TASK_PERIOD_MS));
	}

}
