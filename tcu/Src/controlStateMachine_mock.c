
#include <stdio.h>

#include "cmsis_os.h"
#include "bsp.h"
#include "task.h"
#include "FreeRTOS_CLI.h"
#include "uwfe_debug.h"
#include "string.h"

#include "mainTaskEntry.h"
#include "controlStateMachine_mock.h"

// static const CLI_Command_Definition_t fakeCLICommandDefinition =
// {
// 	"fakeCommand",
// 	"fakeCommand \r\n help description\r\n",
// 	functionToCall,
// 	0 /*Number of parameters*/
// };

HAL_StatusTypeDef stateMachineMockInit()
{
    // if (FreeRTOS_CLIRegisterCommand(&fakeCLICommandDefinition) != pdPASS) {
    //     return HAL_ERROR;
    // }
    return HAL_OK;
}