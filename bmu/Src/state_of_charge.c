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
- integrate with the task and rtos and stuff
- somethign something lookup table

also idk whats happening but im getting some include errors - will look into later
*/

#define SOC_TASK_PERIOD 200 
#define SOC_TASK_ID 7

#define CELL_HIGH_VOLTAGE_LOOKUP_CUTOFF 4.0f
#define CELL_LOW_VOLTAGE_LOOKUP_CUTOFF 3.28f

#define SEGMENT_HIGH_VOLTAGE_LOOKUP_CUTOFF (CELL_HIGH_VOLTAGE_LOOKUP_CUTOFF * CELLS_PER_BOARD * NUM_BOARDS_PER_SEGMENT) //When the segment reaches this threshold, the soc algorithm will be using the integration method exclusively
#define SEGMENT_LOW_VOLTAGE_LOOKUP_CUTOFF (CELL_LOW_VOLTAGE_LOOKUP_CUTOFF * CELLS_PER_BOARD * NUM_BOARDS_PER_SEGMENT) //When the segment reaches this threshold, the soc algorithm will start weighing the lookup table method

#define SOC_HIGH_VOLTAGE_SOC_CUTOFF (0.942f) // Ramp up to around all cells 4V
#define SOC_LOW_VOLTAGE_SOC_CUTOFF (0.06144f) // Ramp down when all cells around 3V


// Units A-s 152.44898 per cell
static const float TOTAL_CAPACITY = 128050.0f;

static float capacity_startup = 1.0f;

// my variables
typedef struct {
	float pred; // current soc estimate
	float variance;
	float process_noise;
	float measurement_noise;
} UKF_State;
static UKF_State ukf;
// add some code to initialize UKF struct in the task init

typedef struct {
	float sigma_points[3];
} UKF_SigmaPoints;
static UKF_SigmaPoints sigmaPoints;

static HAL_StatusTypeDef getSegmentVoltage(float *segmentVoltage);
static float interpolateLut(float value, float lut_min, float lut_step, uint8_t lutLen, const float lut[]);

float predict_voltage(float soc) { return 0.0f; } // figure this out - ecm?

void ukf_soc(float voltage, float current, float dt)
{
	// Subtract current*time from old SOC to estimate current SOC (just coulomb counting - same as old method)
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

	// somethign potentially weird right now where it's predicting the voltage at the current soc instead of using the real SOC? verify this is correct later
	// ok it's probably correct but i'll keep a comment here to remind me to verify again later

	float cross_covariance =  (spread*(sigmaPoints.sigma_points[1]-v_sigma_mean) +
			   (-spread)*(sigmaPoints.sigma_points[2]-v_sigma_mean))/3.0f;

	innov_covariance = (innov_covariance != 0.0f) ? innov_covariance : 1.0f; // prevent div by 0 (shouldnt happen but you never know)
	float kalman_gain = cross_covariance / innov_covariance;
	soc = soc + kalman_gain * (voltage - v_sigma_mean); // update SOC prediction with magic
	soc = soc > 1.0f ? 1.0f : soc;
	soc = soc < 0.0f ? 0.0f : soc;

	ukf.pred = soc;
	ukf.variance = ukf.variance - kalman_gain * innov_covariance * kalman_gain;
}


// The SOC task still needs a lot of rewriting
// needs: 
// parameter initialization
// initial state thing
// cleaning up and other stuff
void socTask(void *pvParamaters)
{
	// Wait until segment voltage is set
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

	// Initially set them to be about equivalent
	float v_soc = compute_voltage_soc();
	float i_soc = v_soc;
	capacity_startup = v_soc * TOTAL_CAPACITY;
	DEBUG_PRINT("Initial SOC: %f %% \n", v_soc * 100.0f);

	if (registerTaskToWatch(SOC_TASK_ID, 2*pdMS_TO_TICKS(SOC_TASK_PERIOD), false, NULL) != HAL_OK)
	{
		ERROR_PRINT("ERROR: Failed to init SOC task, suspending SOC task\n");
		while(1);
	}
	while(1)
	{
		v_soc = compute_voltage_soc();
		i_soc = compute_current_soc();

		//DEBUG_PRINT("SOC: V: %f %%, I: %f %% \n", v_soc * 100.0f, i_soc*100.0f);
		
		float voltage_weight = 1.0f;
		if (v_soc >= SOC_HIGH_VOLTAGE_SOC_CUTOFF)
		{
			float current_weight = (1.0f - v_soc)/(1.0f - SOC_HIGH_VOLTAGE_SOC_CUTOFF);
			voltage_weight = 1.0f - current_weight;
		}
		else if(v_soc >= SOC_LOW_VOLTAGE_SOC_CUTOFF)
		{
			voltage_weight = 0.0f;
		}
		else
		{
			float current_weight = (v_soc - 0.0f)/(SOC_LOW_VOLTAGE_SOC_CUTOFF - 0.0f);
			voltage_weight = 1.0f - current_weight;
		}
		
		// Clamp voltage weight
		voltage_weight = voltage_weight > 1.0f ? 1.0f : voltage_weight;
		voltage_weight = voltage_weight < 0.0f ? 0.0f : voltage_weight;

		float soc = (v_soc * voltage_weight) + (i_soc * (1.0f-voltage_weight));
		//DEBUG_PRINT("SOC: %f, v_soc: %f, i_soc: %f \n", soc, v_soc, i_soc);
		StateBatteryChargeHV = soc * 100.0f;
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

static HAL_StatusTypeDef getSegmentVoltage(float *segmentVoltage)
{
	float temp = 0.0f;
	HAL_StatusTypeDef ret = getAdjustedPackVoltage(&temp);
	*segmentVoltage = (temp / (float)NUM_SEGMENTS);
	return ret;
}
