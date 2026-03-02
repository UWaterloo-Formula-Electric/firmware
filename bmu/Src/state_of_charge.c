#include "state_of_charge.h"
#include "ltc_chip.h"
#include "state_of_charge_data.h"
#include "batteries.h"
#include "debug.h"
#include "watchdog.h"
#include <math.h>

/*
Harry Lu - State of charge estimation WIP
So far:
- implemented a basic coulomb counting method (same as before - will add additional prediction method once tractive locks in)
- I know the code / logic for that existed in this file before but I rewrote for redundancy /c alrity - will clean up file once done
- added Kalman gain with 3 sigma points 
- added some structs needed

TO DO:
- clean up code
- figure out how to predict voltage (ECM???? tractive lock in)
- somethign something lookup table
*/

#define SOC_TASK_PERIOD 200 
#define SOC_TASK_ID 7

#define CELL_HIGH_VOLTAGE_LOOKUP_CUTOFF 4.0f
#define CELL_LOW_VOLTAGE_LOOKUP_CUTOFF 3.28f

#define SEGMENT_HIGH_VOLTAGE_LOOKUP_CUTOFF (CELL_HIGH_VOLTAGE_LOOKUP_CUTOFF * CELLS_PER_BOARD * NUM_BOARDS_PER_SEGMENT) //When the segment reaches this threshold, the soc algorithm will be using the integration method exclusively
#define SEGMENT_LOW_VOLTAGE_LOOKUP_CUTOFF (CELL_LOW_VOLTAGE_LOOKUP_CUTOFF * CELLS_PER_BOARD * NUM_BOARDS_PER_SEGMENT) //When the segment reaches this threshold, the soc algorithm will start weighing the lookup table method


// Units A-s 152.44898 per cell
static const float TOTAL_CAPACITY = 128050.0f;

// my variables
typedef struct {
	float pred; // current soc estimate - stored as a value between 0 and 1
	float variance;
	float process_noise;
	float measurement_noise;
} UKF_State;
static UKF_State ukf;

typedef struct {
	float sigma_points[3];
} UKF_SigmaPoints;
static UKF_SigmaPoints sigmaPoints;

static HAL_StatusTypeDef getSegmentVoltage(float *segmentVoltage);
static float interpolateLut(float value, float lut_min, float lut_step, uint8_t lutLen, const float lut[]);
static float compute_voltage_soc(void);

float predict_voltage(float soc) { return 0.0f; } // figure this out - ecm?

void ukf_soc(float voltage, float current, float dt)
{
	// Subtract current*time from old SOC to estimate current SOC (coulomb counting - same as old method)
	float soc = ukf.pred;
	float dSOC = current * dt / TOTAL_CAPACITY;
	soc -= dSOC;
	ukf.variance += ukf.process_noise;
	soc = soc > 1.0f ? 1.0f : soc;
	soc = soc < 0.0f ? 0.0f : soc;

	// Predict voltages at sigma points
	float spread = sqrtf(ukf.variance);
	sigmaPoints.sigma_points[0] = predict_voltage(soc);
	sigmaPoints.sigma_points[1] = predict_voltage(soc + spread);
	sigmaPoints.sigma_points[2] = predict_voltage(soc - spread);
	float v_sigma_mean = (sigmaPoints.sigma_points[0] + sigmaPoints.sigma_points[1] + sigmaPoints.sigma_points[2]) / 3.0f;

	// Kalman gain
	float innov_covariance = ((sigmaPoints.sigma_points[0]-v_sigma_mean)*(sigmaPoints.sigma_points[0]-v_sigma_mean) +
			  (sigmaPoints.sigma_points[1]-v_sigma_mean)*(sigmaPoints.sigma_points[1]-v_sigma_mean) +
			  (sigmaPoints.sigma_points[2]-v_sigma_mean)*(sigmaPoints.sigma_points[2]-v_sigma_mean))/3.0f +
			  ukf.measurement_noise; // looks complicated but it's just variance

	float cross_covariance =  (spread*(sigmaPoints.sigma_points[1]-v_sigma_mean) +
			   (-spread)*(sigmaPoints.sigma_points[2]-v_sigma_mean))/3.0f;

	innov_covariance = (innov_covariance != 0.0f) ? innov_covariance : 1.0f; // prevent div by 0 (shouldnt happen but you never know)
	float kalman_gain = cross_covariance / innov_covariance;
	soc = soc + kalman_gain * (voltage - v_sigma_mean); // update SOC prediction with magic - keep in mind that this value is in percent of total capacity
	soc = soc > 1.0f ? 1.0f : soc;
	soc = soc < 0.0f ? 0.0f : soc;

	ukf.pred = soc;
	ukf.variance = ukf.variance - kalman_gain * innov_covariance * kalman_gain;
	ukf.variance = ukf.variance < 1e-6f ? 1e-6f : ukf.variance;
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
		float voltage, current;
		getSegmentVoltage(&voltage);
		current = //some current function - look into

		ukf_soc(voltage, current, SOC_TASK_PERIOD / 1000.0f);
		//DEBUG_PRINT("SOC: %f, v_soc: %f, i_soc: %f \n", soc, v_soc, i_soc);
		StateBatteryChargeHV = ukf.pred * 100.0f;
		watchdogTaskCheckIn(SOC_TASK_ID);
		vTaskDelay(SOC_TASK_PERIOD);
	}
}


static float interpolateLut(float value, float lut_min, float lut_step, uint8_t lutLen, const float lut[])
{
	size_t lowIndex = (value - lut_min)/lut_step;
    if (lowIndex < 0)
    {
        return lut[0];
    }
    else if (lowIndex >= lutLen-1) //Can not interpolate with last value in LUT
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
