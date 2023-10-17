#include "state_of_charge.h"
#include "ltc_chip.h"
#include "state_of_charge_data.h"
#include "batteries.h"
#include "debug.h"
#include "watchdog.h"

#define SOC_TASK_PERIOD 200 
#define SOC_TASK_ID 7

#define SEGMENT_HIGH_VOLTAGE_LOOKUP_CUTOFF 56.f //When the segment reaches 56V, the soc algorithm will be using the integration method exclusively
#define SEGMENT_LOW_VOLTAGE_LOOKUP_CUTOFF 46.f //When the segment reaches 46V, the soc algorithm will start weighing the lookup table method

#define SOC_HIGH_VOLTAGE_SOC_CUTOFF (0.942f) // Ramp up to around all cells 4V
#define SOC_LOW_VOLTAGE_SOC_CUTOFF (0.06144f) // Ramp down when all cells around 3V


// Units A-s
static const float TOTAL_CAPACITY = 74700.0f;

static float capacity_startup = 1.0f;

// Units A-s
static volatile float IBus_integrated = 0.0f;

static HAL_StatusTypeDef getSegmentVoltage(float *segmentVoltage);
static float interpolateLut(float value, float lut_min, float lut_step, uint8_t lutLen, const float lut[]);

// In amp seconds
void integrate_bus_current(float IBus, float period_ms)
{
	IBus_integrated += IBus * (period_ms/1000.0);
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

static float compute_current_soc(void)
{
	float capacity = capacity_startup - IBus_integrated;
	float soc = capacity/TOTAL_CAPACITY;
	soc = soc > 1.0f ? 1.0f : soc;
	soc = soc < 0.0f ? 0.0f : soc;
	return soc;
}

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
	*segmentVoltage = (temp / (float)NUM_BOARDS);
	return ret;
}
