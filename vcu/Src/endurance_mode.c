#include "endurance_mode.h"
#include "FreeRTOS.h"
#include "stm32f7xx_hal.h"
#include "stdbool.h"
#include "vcu_F7_can.h"
#include "cmsis_os.h"
#include "watchdog.h"
#include "motorController.h"
#include "uwfe_debug.h"

#define ENDURANCE_MODE_TASK_ID 2
#define ENDURANCE_MODE_TASK_PERIOD (500)
#define ENDURANCE_MODE_FLAG_BIT (0)
// Reminder these are per motor
#define DISCHARGE_CURRENT_MAX_A (30.0f)
#define DISCHARGE_CURRENT_MIN_A (5.0f)
#define SATURATION_INTEGRAL_BUFFER (2.0f)

static float initial_soc = 0.0f;
static uint32_t num_laps = 0;
static uint32_t num_laps_to_complete = NUMBER_OF_LAPS_TO_COMPLETE_DEFAULT*(ENDURANCE_MODE_BUFFER);
static bool in_endurance_mode = false;
static const float em_kP = 200.0f;
static const float em_kI = 0.2f;
extern osThreadId enduranceModeHandle;

void endurance_mode_EM_callback(void)
{
	static bool has_set_initial_soc = false;
	if(!has_set_initial_soc)
	{	
		initial_soc = StateBatteryChargeHV/100.0f;
		has_set_initial_soc = true;
	}
}

void set_lap_limit(uint32_t laps)
{
	num_laps_to_complete = laps;
}

void trigger_lap(void)
{
	num_laps++;
	xTaskNotifyGive(enduranceModeHandle);
}

void toggle_endurance_mode(void)
{
	in_endurance_mode = !in_endurance_mode;
}

float clamp_motor_current(float current)
{
	if(current > DISCHARGE_CURRENT_MAX_A)
	{
		return DISCHARGE_CURRENT_MAX_A;
	}
	else if(current < DISCHARGE_CURRENT_MIN_A)
	{
		return DISCHARGE_CURRENT_MIN_A;
	}
	return current;
}

static HAL_StatusTypeDef compute_discharge_limit(float * current_limit)
{
	static float last_output_current = DISCHARGE_CURRENT_MAX_A;
	static float error_accum = 0.0f;

	float soc = StateBatteryChargeHV/100.0f;
	// Amount of SoC we should be at to end with 5%
	float expected_soc = (float)(num_laps_to_complete - num_laps)/(float)num_laps_to_complete*initial_soc; 
	float error = expected_soc - soc;
	
	// We want to prevent integral windup when in saturation region
	if(last_output_current < DISCHARGE_CURRENT_MAX_A - SATURATION_INTEGRAL_BUFFER
			&& last_output_current > DISCHARGE_CURRENT_MIN_A + SATURATION_INTEGRAL_BUFFER)
	{
		// integral error, dt = 1 as our time is 1 lap
		error_accum += error;
	}
	
	float output_current = last_output_current - (error*em_kP + error_accum*em_kI);
	*current_limit = clamp_motor_current(output_current);	
	last_output_current = *current_limit;
	return HAL_OK;
}

void enduranceModeTask(void *pvParameters)
{
	if (registerTaskToWatch(ENDURANCE_MODE_TASK_ID, 2*pdMS_TO_TICKS(ENDURANCE_MODE_TASK_PERIOD), false, NULL) != HAL_OK)
	{
		ERROR_PRINT("ERROR: Failed to init endurance mode task, suspending endurance mode task\n");
		while(1);
	}
	while(1)
	{
		uint32_t wait_flag = ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS(ENDURANCE_MODE_TASK_PERIOD/2));
		if(wait_flag & (1U << ENDURANCE_MODE_FLAG_BIT))
		{
			float current_limit = 0.0f;
			// We are in endurance mode
			if(in_endurance_mode)
			{
				HAL_StatusTypeDef ret = compute_discharge_limit(&current_limit);
				if(ret != HAL_OK)
				{
					ERROR_PRINT("Failed to compute discharge limit for endurance mode\n");
					current_limit = DISCHARGE_CURRENT_LIMIT_DEFAULT;
				}
			}
			else
			{
				current_limit = DISCHARGE_CURRENT_LIMIT_DEFAULT;
			}
			setDischargeCurrentLimit(current_limit);
		}
		else
		{
			// The flag was never actually set, we just hit the timeout	
		}
		watchdogTaskCheckIn(ENDURANCE_MODE_TASK_ID);
		vTaskDelay(ENDURANCE_MODE_TASK_PERIOD);
	}

}
