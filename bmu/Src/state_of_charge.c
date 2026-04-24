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


// Units A-s 152.44898 per cell
static const float TOTAL_CAPACITY = 128050.0f;
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

	static const float TEMP_LUT[8] = {-20.0f, -10.0f, 0.0f, 10.0f, 25.0f, 35.0f, 45.0f, 55.0f};
	static const float OCV_LUT[21][8] = {
		// {-20C, -10C, 0C, 10C, 25C, 35C, 45C, 55C}
		{3.2601, 3.1887, 3.1209, 3.0276, 2.8214, 2.7425, 2.7013, 2.6878},  // SOC=0%
		{3.3473, 3.2859, 3.2302, 3.1756, 3.0720, 3.0507, 3.0393, 3.0368},  // SOC=5%
		{3.4243, 3.3776, 3.3273, 3.2752, 3.2054, 3.1918, 3.1845, 3.1820},  // SOC=10%
		{3.4933, 3.4502, 3.4141, 3.3718, 3.3089, 3.2926, 3.2837, 3.2797},  // SOC=15%
		{3.5335, 3.5052, 3.4823, 3.4518, 3.4045, 3.3893, 3.3789, 3.3715},  // SOC=20%
		{3.5812, 3.5457, 3.5245, 3.5082, 3.4808, 3.4689, 3.4596, 3.4525},  // SOC=25%
		{3.6269, 3.6021, 3.5748, 3.5506, 3.5251, 3.5195, 3.5155, 3.5134},  // SOC=30%
		{3.6699, 3.6540, 3.6315, 3.6062, 3.5783, 3.5691, 3.5636, 3.5597},  // SOC=35%
		{3.7163, 3.6989, 3.6836, 3.6641, 3.6328, 3.6244, 3.6191, 3.6162},  // SOC=40%
		{3.7619, 3.7459, 3.7314, 3.7165, 3.6948, 3.6837, 3.6772, 3.6742},  // SOC=45%
		{3.8041, 3.7896, 3.7774, 3.7656, 3.7494, 3.7485, 3.7470, 3.7490},  // SOC=50%
		{3.8410, 3.8305, 3.8203, 3.8103, 3.7975, 3.7980, 3.7974, 3.8002},  // SOC=55%
		{3.8730, 3.8668, 3.8597, 3.8515, 3.8416, 3.8419, 3.8418, 3.8445},  // SOC=60%
		{3.9094, 3.9064, 3.9023, 3.8955, 3.8855, 3.8811, 3.8798, 3.8813},  // SOC=65%
		{3.9616, 3.9650, 3.9596, 3.9522, 3.9433, 3.9424, 3.9404, 3.9402},  // SOC=70%
		{4.0148, 4.0257, 4.0186, 4.0097, 3.9981, 3.9978, 3.9955, 3.9963},  // SOC=75%
		{4.0442, 4.0594, 4.0602, 4.0570, 4.0524, 4.0539, 4.0526, 4.0543},  // SOC=80%
		{4.0590, 4.0707, 4.0726, 4.0734, 4.0745, 4.0760, 4.0763, 4.0774},  // SOC=85%
		{4.0746, 4.0811, 4.0827, 4.0833, 4.0846, 4.0858, 4.0861, 4.0869},  // SOC=90%
		{4.0903, 4.0972, 4.0992, 4.0993, 4.1015, 4.1020, 4.1016, 4.1022},  // SOC=95%
		{4.1767, 4.1825, 4.1795, 4.1745, 4.1837, 4.1763, 4.1697, 4.1687},  // SOC=100%
	};

	// Clamp soc between 0 and 1 just in case
	if(soc > 1.0f) {
		soc = 1.0f;
	}
	else if (soc < 0.0f) {
		soc = 0.0f;
	}

	// Get the index and fraction of the soc% in the LUT
	float soc_idx_f = soc * 20.0f;
	uint8_t soc_idx = (uint8_t)(soc_idx_f);
	
	// Clamp index to 20 (so that soc_idx+1=21 max and we don't go out of bounds)
	if (soc_idx >= 21) {
		soc_idx = 20;
	}

	// Get the fraction for interpolation between the two closest SOC% in the LUT
	float soc_frac = (soc_idx_f - (float)soc_idx);
	// If soc is 100%, then we must set the fraction to 1 so that is uses the last row of the LUT
	if(soc >= 1.0f) {
		soc_frac = 1.0f;
	}

	// Get the temperature index and fraction for interpolation
	uint8_t temp_idx = 0;
	float temp_frac = 0.0f;

	if (avg_temp <= TEMP_LUT[0]) {
		temp_idx = 0;
		temp_frac = 0.0f;
	} else if (avg_temp >= TEMP_LUT[7]) {
		temp_idx = 6;
		temp_frac = 1.0f;
	} else {
		// Loop through the TEMP_LUT
		for (uint8_t i = 0; i < 7; i++) {
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