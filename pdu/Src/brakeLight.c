#include "brakeLight.h"
#include "bsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pdu_can.h"
#include "debug.h"

#define BRAKE_LIGHT_ON_THRESHOLD_DEFAULT (15.0f)
#define BRAKE_TASK_PERIOD_MS 300

static float brake_light_on_threshold = BRAKE_LIGHT_ON_THRESHOLD_DEFAULT;

bool isBrakePressed(uint32_t brakePercent)
{
    return (BrakePercent > brake_light_on_threshold);
}

void CAN_Msg_VCU_Data_Callback()
{
    if (isBrakePressed(BrakePercent)) {
        BRAKE_LIGHT_ENABLE;
    } else {
        BRAKE_LIGHT_DISABLE;
    }
}

HAL_StatusTypeDef setBrakeLightOnThreshold(float new_brake_light_on_threshold)
{
    if(new_brake_light_on_threshold < 0.0f || new_brake_light_on_threshold > 100.0f)
	{
		ERROR_PRINT("New brake light on threshhold value out of range. Should be [0, 100]\r\n");
		return HAL_ERROR;
	}
    brake_light_on_threshold = new_brake_light_on_threshold;
    DEBUG_PRINT("Set brake_light_on_threshold to: %f\r\n", brake_light_on_threshold);
	return HAL_OK;
}

float getBrakeLightOnThreshold(void)
{
    return brake_light_on_threshold;
}
