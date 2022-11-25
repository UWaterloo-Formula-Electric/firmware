
#include <stdio.h>

#include "cmsis_os.h"
#include "bsp.h"
#include "task.h"
#include "FreeRTOS_CLI.h"
#include "debug.h"
#include "string.h"

#include "mainTaskEntry.h"
#include "controlStateMachine_mock.h"
#include "controlStateMachine.h"
#include "canReceive.h"

BaseType_t setTractionControl (char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
	BaseType_t paramLen;
	const char * tcVal = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
	uint8_t tc = 0;

    if(strlen(tcVal) > 1 || tcVal[0] < '0' || tcVal[0] > '1')
	{
		COMMAND_OUTPUT("Input Error: expecting [1,0]\r\n");
		return pdFALSE;
	}
	if (tcVal[0] == '1')
	{
		tc = 1;
	}
    if(fsmGetState(&DCUFsmHandle) != STATE_EM_Enable)
	{
        COMMAND_OUTPUT("Error: Need to be at EM to toggle TC\r\n");
        return pdFALSE;
    }

	if(tc == getTC())
	{
		if(tc)
		{
			COMMAND_OUTPUT("TC already on.\r\n");
		}
		else
		{
			COMMAND_OUTPUT("TC already off.\r\n");
		}
	}
	else
	{
		fsmSendEvent(&DCUFsmHandle, EV_TC_Toggle,portMAX_DELAY);
	}
	return pdFALSE;
}

static const CLI_Command_Definition_t setTractionControlCommandDefinition =
{
	"tc",
	"tc <0,1>:\r\n Set Traction control on or off\r\n",
	setTractionControl,
	1 /*Number of parameters*/
};

HAL_StatusTypeDef stateMachineMockInit()
{
    if (FreeRTOS_CLIRegisterCommand(&setTractionControlCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    return HAL_OK;
}