#include "sensors.h"

#include "FreeRTOS.h"
#include "task.h"
#include "bsp.h"
#include "stm32f0xx_hal_tim.h"
#include "watchdog.h"
#include "debug.h"
#include "userCan.h"
#include "stdbool.h"

#include AUTOGEN_HEADER_NAME(BOARD_NAME)

// Task Information
#define POLL_SENSORS_TASK_ID 2
#define POLL_SENSORS_PERIOD_MS 500


// Encoder Information
#define ENCODER_COUNTER (__HAL_TIM_GET_COUNTER(&ENCODER_TIM_HANDLE))
#define ENCODER_PULSES_PER_REVOLUTION (3)
#define WHEEL_DIAMETER_MM (525)

// About a 0.6% error due to integer rounding of PI
// Increasing scale of PI does not improve accuracy
#define PI_SCALE (100)
#define PI_SCALED (314)
#define ENCODER_COUNT_TO_MM(count) ((count)*(((WHEEL_DIAMETER_MM*PI_SCALED)/PI_SCALE)/12))
#define ENCODER_COUNT_TO_RADS_S(delta_count, period) (((2*PI_SCALED*delta_count)/(ENCODER_PULSES_PER_REVOLUTION*PI_SCALE))/period)

typedef struct {
	volatile uint32_t encoder_counts;
	volatile uint32_t encoder_mm;
	volatile float    encoder_speed;
} sensors_data_S;


static sensors_data_S sensors_data;

static void transmit_sensor_values(void);

static void poll_encoder(void)
{
	static uint16_t last_count = 0;

	// TIM3 uses lower 16 bits
	uint16_t current_count = ENCODER_COUNTER;
	uint16_t count_diff = current_count - last_count;
	sensors_data.encoder_counts += ((uint32_t)current_count - (uint32_t)last_count);
	sensors_data.encoder_mm = ENCODER_COUNT_TO_MM(sensors_data.encoder_counts);
	sensors_data.encoder_speed = ENCODER_COUNT_TO_RADS_S(count_diff, (float)(POLL_SENSORS_PERIOD_MS)/1000.0f);
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
		poll_encoder();

		transmit_sensor_values();

        watchdogTaskCheckIn(POLL_SENSORS_TASK_ID);
		
		// Always poll at almost exactly PERIOD
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(POLL_SENSORS_PERIOD_MS));
		xLastWakeTime = xTaskGetTickCount();
	}
}

static void transmit_encoder(void)
{	
#if (BOARD_ID == ID_WSBFL)
	FL_WheelDistance = sensors_data.encoder_mm;
	sendCAN_WSBFL_Sensors();
#elif (BOARD_ID == ID_WSBFR)
	FR_WheelDistance = sensors_data.encoder_mm;
	sendCAN_WSBFR_Sensors();
#endif
}

static void transmit_speed(void)
{
#if (BOARD_ID == ID_WSBFL)
	SpeedWheelLeftFront = sensors_data.encoder_speed;
	sendCAN_WSBFL_WheelData();
#elif (BOARD_ID == ID_WSBFR)
	SpeedWheelRightFront = sensors_data.encoder_speed;
	sendCAN_WSBFR_WheelData();
#endif
	
}

static void transmit_sensor_values(void)
{
	// Send over CAN
	transmit_encoder();
	transmit_speed();
}

uint32_t sensor_encoder_count(void)
{
	return sensors_data.encoder_counts;
}

uint32_t sensor_encoder_mm(void)
{
	return sensors_data.encoder_mm;
}

float sensor_encoder_speed(void)
{
	return sensors_data.encoder_speed;
}

HAL_StatusTypeDef sensors_init(void)
{
    if(HAL_TIM_IC_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
	{
		ERROR_PRINT("Failed to start Encoder timer\n");
		return HAL_ERROR;	
	}
	return HAL_OK;
}
