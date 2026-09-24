#include "state_of_charge.h"
#include "ltc_chip.h"
#include "state_of_charge_data.h"
#include "batteries.h"
#include "debug.h"
#include "watchdog.h"
#include "bmu_can.h"

#define SOC_TASK_PERIOD 200 
#define SOC_TASK_ID 7
#define SOC_TASK_PERIOD_S ((float)SOC_TASK_PERIOD / 1000.0f)

/* PID stays enabled; gains are 0 until tuned (correction term is zero). */
#define SOC_PID_KP 0.0f
#define SOC_PID_KI 0.0f
#define SOC_PID_KD 0.0f
#define SOC_PID_INTEGRAL_MAX 0.05f

#define CELL_HIGH_VOLTAGE_LOOKUP_CUTOFF 4.0f
#define CELL_LOW_VOLTAGE_LOOKUP_CUTOFF 3.28f

#define SEGMENT_HIGH_VOLTAGE_LOOKUP_CUTOFF (CELL_HIGH_VOLTAGE_LOOKUP_CUTOFF * CELLS_PER_BOARD * NUM_BOARDS_PER_SEGMENT) //When the segment reaches this threshold, the soc algorithm will be using the integration method exclusively
#define SEGMENT_LOW_VOLTAGE_LOOKUP_CUTOFF (CELL_LOW_VOLTAGE_LOOKUP_CUTOFF * CELLS_PER_BOARD * NUM_BOARDS_PER_SEGMENT) //When the segment reaches this threshold, the soc algorithm will start weighing the lookup table method


// BAK INR2170-45D: 4.5 [A-h] -> 16200 [A-s]
// 3 cells in parallel (140s3p)
static const float TOTAL_CAPACITY = 48600.0f; // [A-s]
static SemaphoreHandle_t IBus_mutex;

static float soc_estimate = 0.0f; /* 0-1 */
static float pid_integral = 0.0f;
static float pid_last_error = 0.0f;

static volatile float IBus_integrated = 0.0f;

static HAL_StatusTypeDef getSegmentVoltage(float *segmentVoltage);
static float interpolateLut(float value, float lut_min, float lut_step, uint8_t lutLen, const float lut[]);
static float compute_voltage_soc(void);
static void update_soc(float voltage, float current_integrated);
void socTask(void *pvParamaters);
static float get_avg_temp(void);
static HAL_StatusTypeDef consume_integrated_current(float *current);

static float predict_voltage(float soc, float avg_temp) {
	// We have the LUT of the OCV, and use bilinear interpolation to calculate in-between points

	// Clamp soc between 0 and 1 just in case
	soc = soc > 1.0f ? 1.0f : soc;
	soc = soc < 0.0f ? 0.0f : soc;

	// Get the index and fraction of the soc% in the LUT
	float soc_idx_f = soc * (float)(OCV_LUT_SOC_POINTS - 1);
	uint8_t soc_idx = (uint8_t)(soc_idx_f);
	soc_idx = soc_idx > OCV_LUT_SOC_POINTS - 2 ? OCV_LUT_SOC_POINTS - 2 : soc_idx;
	
	// Get the fraction for interpolation between the two closest SOC% in the LUT
	float soc_frac = (soc_idx_f - (float)soc_idx);
	// If soc is 100%, then we must set the fraction to 1 so that is uses the last row of the LUT
	soc_frac = soc >= 1.0f ? 1.0f : soc_frac;

	// Get the temperature index and fraction for interpolation
	uint8_t temp_idx = 0;
	float temp_frac = 0.0f;

	if (avg_temp <= TEMP_LUT[0]) {
		temp_idx = 0;
		temp_frac = 0.0f;
	} else if (avg_temp >= TEMP_LUT[TEMP_LUT_LEN - 1]) {
		temp_idx = TEMP_LUT_LEN - 2;
		temp_frac = 1.0f;
	} else {
		// Loop through the TEMP_LUT
		for (uint8_t i = 0; i < TEMP_LUT_LEN - 1; i++) {
			if (avg_temp >= TEMP_LUT[i] && avg_temp < TEMP_LUT[i+1]) {
				temp_idx = i;
				temp_frac = (avg_temp - TEMP_LUT[i]) / (TEMP_LUT[i+1] - TEMP_LUT[i]);
				break;
			}
		}
	}

	// Bilinear Interpolation
	float val00 = OCV_LUT[soc_idx][temp_idx];
	float val10 = OCV_LUT[soc_idx+1][temp_idx];
	float val01 = OCV_LUT[soc_idx][temp_idx+1];
	float val11 = OCV_LUT[soc_idx+1][temp_idx+1];

	// Interpolate SOC first, then temperature
	float interp_soc_0 = val00 + soc_frac * (val10 - val00);
	float interp_soc_1 = val01 + soc_frac * (val11 - val01);

	return interp_soc_0 + temp_frac * (interp_soc_1 - interp_soc_0);
}

static void update_soc(float voltage, float current_integrated)
{
	voltage = voltage / (float)(CELLS_PER_BOARD * NUM_BOARDS_PER_SEGMENT);

	float soc = soc_estimate;
	soc -= current_integrated / TOTAL_CAPACITY;

	float v_pred = predict_voltage(soc, get_avg_temp());
	float error = voltage - v_pred; /* +error: pack higher than OCV(soc) -> raise SOC */

	pid_integral += error * SOC_TASK_PERIOD_S;
	if (pid_integral > SOC_PID_INTEGRAL_MAX) {
		pid_integral = SOC_PID_INTEGRAL_MAX;
	} else if (pid_integral < -SOC_PID_INTEGRAL_MAX) {
		pid_integral = -SOC_PID_INTEGRAL_MAX;
	}

	float d_error = (error - pid_last_error) / SOC_TASK_PERIOD_S;
	pid_last_error = error;

	soc += (SOC_PID_KP * error) + (SOC_PID_KI * pid_integral) + (SOC_PID_KD * d_error);
	soc = soc > 1.0f ? 1.0f : soc;
	soc = soc < 0.0f ? 0.0f : soc;
	soc_estimate = soc;
}

static float get_avg_temp(void)
{
	float avg_temp = 0.0f;
	for (int i = 0; i < NUM_TEMP_CELLS; i++) {
		avg_temp += TempChannel[i];
	}
	avg_temp /= NUM_TEMP_CELLS;
	return avg_temp;
}

void socTask(void *pvParamaters)
{
	// Wait until segment voltage is set
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
	soc_estimate = compute_voltage_soc();
	pid_integral = 0.0f;
	pid_last_error = 0.0f;

	DEBUG_PRINT("Initial SOC: %f %% \n", soc_estimate * 100.0f);

	if (registerTaskToWatch(SOC_TASK_ID, 2*pdMS_TO_TICKS(SOC_TASK_PERIOD), false, NULL) != HAL_OK)
	{
		ERROR_PRINT("ERROR: Failed to init SOC task, suspending SOC task\n");
		while(1);
	}

	while(1) {
		float voltage = 0.0f, current_integrated = 0.0f;
		if (getSegmentVoltage(&voltage) == HAL_OK) {
			if (consume_integrated_current(&current_integrated) == HAL_OK) {
				update_soc(voltage, current_integrated);
				StateBatteryChargeHV = soc_estimate * 100.0f;
			}
		}

		//DEBUG_PRINT("SOC: %f, v_soc: %f, i_soc: %f \n", soc, v_soc, i_soc);
		watchdogTaskCheckIn(SOC_TASK_ID);
		vTaskDelay(pdMS_TO_TICKS(SOC_TASK_PERIOD));
	}
}


static float interpolateLut(float value, float lut_min, float lut_step, uint8_t lutLen, const float lut[])
{
	if (value <= lut_min) // Below the table. Converting a negative float to size_t is undefined behaviour
	{
		return lut[0];
	}
	size_t lowIndex = (value - lut_min)/lut_step;
    if (lowIndex >= lutLen-1) //Can not interpolate with last value in LUT
    {
        return lut[lutLen-1];
    }
	//	DEBUG_PRINT("lowIndex : %u\n", lowIndex);
	float lowValue = lut_min + lowIndex*lut_step;
    
    return lut[lowIndex] + (value - lowValue)*(lut[lowIndex+1]-lut[lowIndex])/(lut_step);
}

static float compute_voltage_soc(void)
{
	float soc = 0.0f;
	float segment_voltage = 0.0f;
	const float * soc_lut;
	float lut_min = 0.0f;
	float lut_step = 0.0f;
	float lut_len = 0.0f;
	
	if(getSegmentVoltage(&segment_voltage) != HAL_OK)
	{
		ERROR_PRINT("Failed to read segment voltage, returning 0V");
		return 0.0f;
	}
//	DEBUG_PRINT("Segment Voltage: %f\n", segment_voltage);

	if(segment_voltage >= SEGMENT_HIGH_VOLTAGE_LOOKUP_CUTOFF)
	{
		soc_lut = highVoltageSocLut;
		lut_min =  HV_SOC_LUT_MIN;
		lut_step = HV_SOC_LUT_STEP;
		lut_len = HV_SOC_LUT_LEN;
	}
	else if(segment_voltage >= SEGMENT_LOW_VOLTAGE_LOOKUP_CUTOFF)
	{
		soc_lut = midVoltageSocLut;
		lut_min =  MID_SOC_LUT_MIN;
		lut_step = MID_SOC_LUT_STEP;
		lut_len = MID_SOC_LUT_LEN;
	}
	else
	{
		soc_lut = lowVoltageSocLut;
		lut_min =  LV_SOC_LUT_MIN;
		lut_step = LV_SOC_LUT_STEP;
		lut_len = LV_SOC_LUT_LEN;	
	}
	soc = interpolateLut(segment_voltage, lut_min, lut_step, lut_len, soc_lut);
	soc = soc > 1.0f ? 1.0f : soc;
	soc = soc < 0.0f ? 0.0f : soc;
	return soc;
}

static HAL_StatusTypeDef getSegmentVoltage(float *segmentVoltage)
{
	float temp = 0.0f;
	HAL_StatusTypeDef ret = getAdjustedPackVoltage(&temp);
	*segmentVoltage = (temp / (float)NUM_SEGMENTS);
	return ret;
}

void integrate_bus_current(float IBus, float period_ms)
{
	if (xSemaphoreTake(IBus_mutex, 0) == pdTRUE) {
        IBus_integrated += IBus * (period_ms / 1000.0f);
        xSemaphoreGive(IBus_mutex);
    }
}

static HAL_StatusTypeDef consume_integrated_current(float *current)
{
	float temp = 0.0f;
	if (xSemaphoreTake(IBus_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
		temp = IBus_integrated;
		IBus_integrated = 0.0f;
		xSemaphoreGive(IBus_mutex);
		*current = temp;
		return HAL_OK;
	} else{
		ERROR_PRINT("Failed to take IBus mutex to consume integrated current\n");
		return HAL_ERROR;
	}
}

HAL_StatusTypeDef initSOC(void)
{
    IBus_mutex = xSemaphoreCreateMutex();
    if (IBus_mutex == NULL) {
        ERROR_PRINT("Failed to create IBus mutex\n");
        return HAL_ERROR;
    }
    return HAL_OK;
}
