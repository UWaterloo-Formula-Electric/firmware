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
#define POLL_SENSORS_PERIOD_MS 10


// Encoder Information
#define ENCODER_COUNTER (__HAL_TIM_GET_COUNTER(&ENCODER_TIM_HANDLE))
// #define ENCODER_PULSES_PER_REVOLUTION (1280.0f*2.0f)
// #define WHEEL_DIAMETER_MM (525)
#define ENCODER_PULSES_PER_REVOLUTION (10240.0f)
#define WHEEL_DIAMETER_MM (406.3f)

// About a 0.6% error due to integer rounding of PI
// Increasing scale of PI does not improve accuracy
#define PI_SCALE (100)
#define PI_SCALED (314)
#define ENCODER_COUNT_TO_MM(count) (((count)*(WHEEL_DIAMETER_MM*PI_SCALED/PI_SCALE))/ENCODER_PULSES_PER_REVOLUTION)
#define ENCODER_COUNT_TO_RADS_S(delta_count, period) (((2*PI_SCALED*(delta_count))/(ENCODER_PULSES_PER_REVOLUTION*PI_SCALE))/(period))

typedef struct {
	volatile uint32_t encoder_counts;
	volatile uint32_t encoder_mm;
	volatile float    encoder_speed;
	volatile int rear_brake_pressure; //newly added
} sensors_data_S;

static sensors_data_S sensors_data;

////brake pressure new stuff, untested

uint32_t ADCVals[NUM_ADC_CHANNELS] = {0};

int map_range(int in, int low, int high, int low_out, int high_out) { //copied from brake and throttle code in the VCU
    if (in < low) {
        in = low;
    } else if (in > high) {
        in = high;
    }
    int in_range = high - low;
    int out_range = high_out - low_out;

    return (in - low) * out_range / in_range + low_out;
}
HAL_StatusTypeDef startADCConversions() {
	if (HAL_ADC_Start_DMA(&ADC_HANDLE, ADCVals, NUM_ADC_CHANNELS) != HAL_OK)
	{
		ERROR_PRINT("Failed to start ADC DMA conversions\n");
		//Error_Handler(); //idk where this one is from
		return HAL_ERROR;
	}
	return HAL_OK;
}
int calculateBrakePressure(uint32_t bp) {
	return map_range(bp, BRAKE_PRES_ADC_LOW, BRAKE_PRES_ADC_HIGH, BRAKE_PRES_PSI_LOW, BRAKE_PRES_PSI_HIGH);
}

bool checkBPSRange(uint16_t bp) {
	return (BRAKE_PRES_ADC_LOW-BRAKE_PRES_DEADZONE <= bp && bp <= BRAKE_PRES_ADC_HIGH+BRAKE_PRES_DEADZONE);
}
int getBrakePressure() {
	uint32_t rawBPres = ADCVals[BRAKE_PRES_INDEX];
	if(!checkBPSRange(rawBPres)) {
		DEBUG_PRINT("Brake pressure reading out of range!\n");
		return -1;
	}
	return calculateBrakePressure(rawBPres);
}
static void poll_brake_pressure(void)
{
	sensors_data.rear_brake_pressure = getBrakePressure();
}
static void transmit_brake_pressure(void) {

}
////

static void transmit_sensor_values(void);

static void poll_encoder(void)
{
	static uint16_t last_count = 0;

	// TIM3 uses lower 16 bits
	uint16_t current_count = ENCODER_COUNTER;
	uint32_t count_diff = (uint16_t)(current_count - last_count);
	sensors_data.encoder_counts += count_diff;
	sensors_data.encoder_mm = ENCODER_COUNT_TO_MM(sensors_data.encoder_counts);
	sensors_data.encoder_speed = ENCODER_COUNT_TO_RADS_S(count_diff, (float)(POLL_SENSORS_PERIOD_MS)/1000.0f);
	last_count = current_count;
}

void pollSensorsTask(void const * argument)
{
    if (registerTaskToWatch(POLL_SENSORS_TASK_ID, 5*pdMS_TO_TICKS(POLL_SENSORS_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register sensors task with watchdog!\n");
        Error_Handler();
    }
	
	TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while(1)
	{
		poll_encoder();
		poll_brake_pressure();

		transmit_sensor_values();

        watchdogTaskCheckIn(POLL_SENSORS_TASK_ID);
		
		// Always poll at almost exactly PERIOD
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(POLL_SENSORS_PERIOD_MS));
	}
}

static void transmit_encoder(void)
{	
#if (BOARD_ID == ID_WSBFL)
	FL_Speed_RAD_S = sensors_data.encoder_speed;
	FL_WheelDistance = sensors_data.encoder_mm;
	sendCAN_WSBFL_Sensors();
#elif (BOARD_ID == ID_WSBFR)
	FR_Speed_RAD_S = sensors_data.encoder_speed;
	FR_WheelDistance = sensors_data.encoder_mm;
	sendCAN_WSBFR_Sensors();
#endif
}

static void transmit_sensor_values(void)
{
	// Send over CAN
	transmit_encoder();
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


