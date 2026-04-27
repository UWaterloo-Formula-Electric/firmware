#include "state_of_charge.h"
#include "ltc_chip.h"
#include "state_of_charge_data.h"
#include "batteries.h"
#include "debug.h"
#include "watchdog.h"
#include "bmu_can.h"
#include <math.h>

#define SOC_TASK_PERIOD 200 
#define SOC_TASK_ID 7

#define CELL_HIGH_VOLTAGE_LOOKUP_CUTOFF 4.0f
#define CELL_LOW_VOLTAGE_LOOKUP_CUTOFF 3.28f

#define SEGMENT_HIGH_VOLTAGE_LOOKUP_CUTOFF (CELL_HIGH_VOLTAGE_LOOKUP_CUTOFF * CELLS_PER_BOARD * NUM_BOARDS_PER_SEGMENT) //When the segment reaches this threshold, the soc algorithm will be using the integration method exclusively
#define SEGMENT_LOW_VOLTAGE_LOOKUP_CUTOFF (CELL_LOW_VOLTAGE_LOOKUP_CUTOFF * CELLS_PER_BOARD * NUM_BOARDS_PER_SEGMENT) //When the segment reaches this threshold, the soc algorithm will start weighing the lookup table method


// BAK INR2170-45D: 4.5 [A-h] -> 16200 [A-s]
// 3 cells in parallel (140s3p)
static const float TOTAL_CAPACITY = 48600.0f; // [A-s]
static SemaphoreHandle_t IBus_mutex;

// my variables
typedef struct {
	float pred; // current soc estimate - stored as a value between 0 and 1
	float variance;
	float process_noise;
	float measurement_noise;
} UKF_State;
static UKF_State ukf;

volatile float IBus_integrated = 0.0f;

static HAL_StatusTypeDef getSegmentVoltage(float *segmentVoltage);
static float interpolateLut(float value, float lut_min, float lut_step, uint8_t lutLen, const float lut[]);
static float compute_voltage_soc(void);
void ukf_soc(float voltage, float current_integrated);
void socTask(void *pvParamaters);
static float get_avg_temp(void);
HAL_StatusTypeDef consume_integrated_current(float *current);

float predict_voltage(float soc, float avg_temp) { 
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

void ukf_soc(float voltage, float current_integrated)
{
	// Subtract current*time from old SOC to estimate current SOC (coulomb counting - same as old method)
	float soc = ukf.pred;
	soc -= current_integrated / TOTAL_CAPACITY;
	ukf.variance += ukf.process_noise;
	float avg_temp = get_avg_temp();
	// Predict voltages at sigma points
	float spread = sqrtf(ukf.variance);
	float sigma_points[3];
	sigma_points[0] = predict_voltage(soc, avg_temp);
	sigma_points[1] = predict_voltage(soc + spread, avg_temp);
	sigma_points[2] = predict_voltage(soc - spread, avg_temp);
	float v_sigma_mean = 0.5f * (sigma_points[1] + sigma_points[2]);

	// Kalman gain
	float innov_covariance = 2.0f * ((sigma_points[0]-v_sigma_mean)*(sigma_points[0]-v_sigma_mean)) +
			  0.5f * ((sigma_points[1]-v_sigma_mean)*(sigma_points[1]-v_sigma_mean)) +
			  0.5f * ((sigma_points[2]-v_sigma_mean)*(sigma_points[2]-v_sigma_mean)) +
			  ukf.measurement_noise; // weighted variance of sigma points

	float cross_covariance =  0.5f * (spread*(sigma_points[1]-v_sigma_mean) +
			   (-spread)*(sigma_points[2]-v_sigma_mean));

	if (innov_covariance < 1e-6f) innov_covariance = 1e-6f; // prevent divide by 0 which hopefully shouldnt happen anyway
	float kalman_gain = cross_covariance / innov_covariance;
	soc = soc + kalman_gain * (voltage - v_sigma_mean); // update SOC prediction with magic - keep in mind that this value is in percent of total capacity
	soc = soc > 1.0f ? 1.0f : soc;
	soc = soc < 0.0f ? 0.0f : soc;

	ukf.pred = soc;
	ukf.variance = ukf.variance - kalman_gain * innov_covariance * kalman_gain;
	ukf.variance = ukf.variance < 1e-6f ? 1e-6f : ukf.variance;
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
	ukf.pred = compute_voltage_soc(); //initialize with LUT values
	ukf.variance = 0.01f; //tune these values with data later
	ukf.process_noise = 0.001f;
	ukf.measurement_noise = 0.01f;
	
	DEBUG_PRINT("Initial SOC: %f %% \n", ukf.pred * 100.0f);

	if (registerTaskToWatch(SOC_TASK_ID, 2*pdMS_TO_TICKS(SOC_TASK_PERIOD), false, NULL) != HAL_OK)
	{
		ERROR_PRINT("ERROR: Failed to init SOC task, suspending SOC task\n");
		while(1);
	}

	while(1) {
		float voltage = 0.0f, current_integrated = 0.0f;
		if (getSegmentVoltage(&voltage) == HAL_OK) {
			if (consume_integrated_current(&current_integrated) == HAL_OK) {
				ukf_soc(voltage, current_integrated);
				StateBatteryChargeHV = ukf.pred * 100.0f;
			}
		}

		//DEBUG_PRINT("SOC: %f, v_soc: %f, i_soc: %f \n", soc, v_soc, i_soc);
		watchdogTaskCheckIn(SOC_TASK_ID);
		vTaskDelay(pdMS_TO_TICKS(SOC_TASK_PERIOD));
	}
}


static float interpolateLut(float value, float lut_min, float lut_step, uint8_t lutLen, const float lut[])
{
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

HAL_StatusTypeDef consume_integrated_current(float *current)
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