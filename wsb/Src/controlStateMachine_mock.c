#include "controlStateMachine_mock.h"
#include "sensors.h"
#include "string.h"
#include "debug.h"
#include "FreeRTOS_CLI.h"
#include "cmsis_os.h"


BaseType_t sensorsCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
	static int8_t index = 0;
	index++;
	if(index == 1)
	{
		COMMAND_OUTPUT("Sensors Data: \n");
		vTaskDelay(1);
		return pdTRUE;
	}
	else if(index == 2)
	{
		COMMAND_OUTPUT("Encoder: %lu counts, %lu mm\n", sensor_encoder_count(), sensor_encoder_mm());
		vTaskDelay(1);
		return pdTRUE;
	}
	else
	{
		index = 0;
	}
    return pdFALSE;
}
static const CLI_Command_Definition_t sensorsCommandDefinition =
{
    "sensors",
    "sensors :\r\n  Print out sensor data\r\n",
    sensorsCommand,
    0 /* Number of parameters */
};

HAL_StatusTypeDef stateMachineMockInit()
{
    if (FreeRTOS_CLIRegisterCommand(&sensorsCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    return HAL_OK;
}
