#include "controlStateMachine_mock.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "string.h"
#include "state_machine.h"
#include "FreeRTOS_CLI.h"

BaseType_t mockCritical(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEvent(&mainFsmHandle, MN_EV_HV_CriticalFailure, portMAX_DELAY);
    return pdFALSE;
}
static const CLI_Command_Definition_t criticalCommandDefinition =
{
    "critical",
    "critical:\r\n  Generates a HV critical failure event\r\n",
    mockCritical,
    0 /* Number of parameters */
};

BaseType_t mockLVCuttoff(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    COMMAND_OUTPUT("Sending lv cuttoff\n");
    fsmSendEventISR(&mainFsmHandle, MN_EV_LV_Cuttoff);
    return pdFALSE;
}
static const CLI_Command_Definition_t lvCuttoffCommandDefinition =
{
    "lvCuttoff",
    "lvCuttoff:\r\n  Generates a lv cuttoff failure event\r\n",
    mockLVCuttoff,
    0 /* Number of parameters */
};

BaseType_t mockEMEnableDisable(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    if (STR_EQ(param, "enable", paramLen)) {
        fsmSendEventISR(&motorFsmHandle, MTR_EV_EM_ENABLE);
    } else if (STR_EQ(param, "disable", paramLen)) {
        fsmSendEventISR(&motorFsmHandle, MTR_EV_EM_DISABLE);
    } else {
        COMMAND_OUTPUT("Unkown parameter\n");
    }

    return pdFALSE;
}
static const CLI_Command_Definition_t emEnableDisableCommandDefinition =
{
    "em",
    "em <enable|disable>:\r\n  Generates either em <enable|disable> event\r\n",
    mockEMEnableDisable,
    1 /* Number of parameters */
};

BaseType_t mockHVEnableDisable(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    if (STR_EQ(param, "enable", paramLen)) {
        fsmSendEventISR(&coolingFsmHandle, COOL_EV_HV_ENABLE);
    } else if (STR_EQ(param, "disable", paramLen)) {
        fsmSendEventISR(&coolingFsmHandle, COOL_EV_HV_DISABLE);
    } else {
        COMMAND_OUTPUT("Unkown parameter\n");
    }

    return pdFALSE;
}
static const CLI_Command_Definition_t hvEnableDisableCommandDefinition =
{
    "hv",
    "hv <enable|disable>:\r\n  Generates either hv <enable|disable> event\r\n",
    mockHVEnableDisable,
    1 /* Number of parameters */
};

BaseType_t mockOvertempWarning(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEventISR(&coolingFsmHandle, COOL_EV_OVERTEMP_WARNING);
    return pdFALSE;
}
static const CLI_Command_Definition_t mockOverTempCommandDefinition =
{
    "overtemp",
    "overtemp:\r\n  Generates overtemp warning event\r\n",
    mockOvertempWarning,
    0 /* Number of parameters */
};

BaseType_t printStates(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    COMMAND_OUTPUT("States:\nCooling: %ld\nMotors: %ld\nMain: %ld\n", fsmGetState(&coolingFsmHandle), fsmGetState(&motorFsmHandle), fsmGetState(&mainFsmHandle));
    return pdFALSE;
}
static const CLI_Command_Definition_t printStateCommandDefinition =
{
    "states",
    "states:\r\n  Output current state of all state machines\r\n",
    printStates,
    0 /* Number of parameters */
};

HAL_StatusTypeDef mockStateMachineInit()
{
    if (FreeRTOS_CLIRegisterCommand(&criticalCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&lvCuttoffCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&emEnableDisableCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&hvEnableDisableCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&mockOverTempCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&printStateCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    return HAL_OK;
}
