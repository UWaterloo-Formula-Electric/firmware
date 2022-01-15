#include "sensors.h"

#include "FreeRTOS.h"
#include "task.h"
#include "bsp.h"
#include "stm32f0xx_hal_tim.h"
#include "watchdog.h"
#include "debug.h"
#include "userCan.h"
#include "stdbool.h"

#define POLL_SENSORS_TASK_ID 2
#define POLL_SENSORS_PERIOD_MS 1000

#define ENCODER_COUNTER (__HAL_TIM_GET_COUNTER(&ENCODER_TIM_HANDLE))

typedef struct {
	volatile uint32_t encoder_counts;
} sensors_data_S;


static sensors_data_S sensors_data;

static void pollEncoder(void)
{
	static uint16_t last_count = 0;

	// TIM3 uses lower 16 bits
	uint16_t current_count = ENCODER_COUNTER;
	sensors_data.encoder_counts += ((uint32_t)current_count - (uint32_t)last_count);
	last_count = current_count;
}

void pollSensorsTask(void const * argument)
{
	TickType_t xLastWakeTime;
    
    if (registerTaskToWatch(POLL_SENSORS_TASK_ID, 5*pdMS_TO_TICKS(POLL_SENSORS_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register sensors task with watchdog!\n");
        Error_Handler();
    }
    
    while(1)
	{

		pollEncoder();

        watchdogTaskCheckIn(POLL_SENSORS_TASK_ID);
		
		xLastWakeTime = xTaskGetTickCount();
		// Always poll at almost exactly PERIOD
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(POLL_SENSORS_PERIOD_MS));
	}
}


uint32_t sensor_encoder_count(void)
{
	return sensors_data.encoder_counts;
}
