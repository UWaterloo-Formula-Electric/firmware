#include "controlStateMachine_mock.h"
#include "debug.h"
#include "string.h"
#include "state_machine.h"
#include "FreeRTOS_CLI.h"
#include "task.h"
#include "cmsis_os.h"
#include "prechargeDischarge.h"
#include "controlStateMachine.h"

BaseType_t printHVMeasurements(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    COMMAND_OUTPUT("IShunt: %f\nVBus: %f\nVBatt: %f\n", getIshunt(), getVBus(), getVBatt());
    return pdFALSE;
}
static const CLI_Command_Definition_t printHVMeasurementsCommandDefinition =
{
    "hvMeasure",
    "hvMeasure:\r\n  Output current hv measurements\r\n",
    printHVMeasurements,
    0 /* Number of parameters */
};

BaseType_t setVBatt(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(param, "%f", &VBatt);
    COMMAND_OUTPUT("Setting VBatt %f\n", VBatt);

    return pdFALSE;
}
static const CLI_Command_Definition_t vBattCommandDefinition =
{
    "VBatt",
    "VBatt <val>:\r\n Set VBatt to val\r\n",
    setVBatt,
    1 /* Number of parameters */
};
BaseType_t setVBus(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(param, "%f", &VBus);
    COMMAND_OUTPUT("Setting VBus %f\n", VBus);

    return pdFALSE;
}
static const CLI_Command_Definition_t vBusCommandDefinition =
{
    "VBus",
    "VBus <val>:\r\n Set VBus to val\r\n",
    setVBus,
    1 /* Number of parameters */
};

BaseType_t setIBus(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    float current;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(param, "%f", &current);

    COMMAND_OUTPUT("setting bus current %f\n", current);
    I_Shunt = current;

    return pdFALSE;
}
static const CLI_Command_Definition_t currentCommandDefinition =
{
    "busCurrent",
    "busCurrent <val>:\r\n Set bus current to val\r\n",
    setIBus,
    1 /* Number of parameters */
};


BaseType_t sendHVFault(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEventISR(&fsmHandle, EV_HV_Fault);
    return pdFALSE;
}
static const CLI_Command_Definition_t hvFaultCommandDefinition =
{
    "hvFault",
    "hvFault:\r\n Send hv fault event\r\n",
    sendHVFault,
    0 /* Number of parameters */
};

BaseType_t fakeHV_ToggleDCU(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEventISR(&fsmHandle, EV_HV_Toggle);
    return pdFALSE;
}
static const CLI_Command_Definition_t hvToggleCommandDefinition =
{
    "hvToggle",
    "hvToggle:\r\n Send hv toggle event\r\n",
    fakeHV_ToggleDCU,
    0 /* Number of parameters */
};

BaseType_t printState(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    COMMAND_OUTPUT("State: %ld\n", fsmGetState(&fsmHandle));
    return pdFALSE;
}
static const CLI_Command_Definition_t printStateCommandDefinition =
{
    "state",
    "state:\r\n  Output current state of state machine\r\n",
    printState,
    0 /* Number of parameters */
};

HAL_StatusTypeDef stateMachineMockInit()
{
    I_Shunt = 0;
    VBatt = 0;

    VBus = 0;
    if (FreeRTOS_CLIRegisterCommand(&printHVMeasurementsCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&vBattCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&vBusCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&currentCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&hvFaultCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&hvToggleCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&printStateCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    return HAL_OK;
}
