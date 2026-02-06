#include "controlStateMachine_mock.h"
#include "sensors.h"
#include "string.h"
#include "debug.h"
#include "FreeRTOS_CLI.h"
#include "cmsis_os.h"


BaseType_t sensorsCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
	COMMAND_OUTPUT("Dummy function, please fill with actual data when sensors are brought up\n");
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
