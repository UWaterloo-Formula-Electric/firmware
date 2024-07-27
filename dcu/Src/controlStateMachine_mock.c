
#include <stdio.h>

#include "cmsis_os.h"
#include "bsp.h"
#include "task.h"
#include "FreeRTOS_CLI.h"
#include "uwfe_debug.h"
#include "string.h"

#include "mainTaskEntry.h"
#include "controlStateMachine_mock.h"
#include "controlStateMachine.h"
#include "canReceive.h"

BaseType_t fakeHvToggle(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
	fsmSendEvent(&DCUFsmHandle, EV_BTN_HV_Toggle, portMAX_DELAY);
	return pdFALSE;
}

static const CLI_Command_Definition_t fakeHvToggleCommandDefinition =
{
	"hv",
	"hv \r\n fake press of HV Toggle\r\n",
	fakeHvToggle,
	0 /*Number of parameters*/
};

HAL_StatusTypeDef stateMachineMockInit()
{
    if (FreeRTOS_CLIRegisterCommand(&fakeHvToggleCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    return HAL_OK;
}