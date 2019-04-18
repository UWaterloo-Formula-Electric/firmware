#include "controlStateMachine_mock.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "string.h"
#include "state_machine.h"
#include "FreeRTOS_CLI.h"
#include "sensors.h"

uint32_t ADC_Buffer[NUM_PDU_CHANNELS];
BaseType_t setChannelCurrent(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    int channelIdx;
    float current;

    const char *idxParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    const char *currentParam = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);

    sscanf(idxParam, "%u", &channelIdx);

    if (channelIdx < 0 || channelIdx >= NUM_PDU_CHANNELS) {
        COMMAND_OUTPUT("channelIdx Index must be between 0 and %d\n", NUM_PDU_CHANNELS);
        return pdFALSE;
    }

    sscanf(currentParam, "%f", &current);
    COMMAND_OUTPUT("Channel %d current = %f A\n", channelIdx, current);

    ADC_Buffer[channelIdx] = current * ADC_TO_AMPS_DIVIDER;
    return pdFALSE;
}
static const CLI_Command_Definition_t setChannelCurrentCommandDefinition =
{
    "channelCurrent",
    "channelCurrent <channel> <current>:\r\n  Set channel <channel> current\r\n",
    setChannelCurrent,
    2 /* Number of parameters */
};

BaseType_t setBusVoltage(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    float voltage;

    const char *voltageParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(voltageParam, "%f", &voltage);
    COMMAND_OUTPUT("Bus voltage = %f V\n", voltage);

    ADC_Buffer[LV_Voltage] = voltage * ADC_TO_VOLTS_DIVIDER;
    return pdFALSE;
}
static const CLI_Command_Definition_t setBusVoltageCommandDefinition =
{
    "busVoltage",
    "busVoltage <voltage>:\r\n  Set bus voltage\r\n",
    setBusVoltage,
    1 /* Number of parameters */
};

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
BaseType_t testOuput(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    FAN_LEFT_ENABLE; 
    HAL_Delay(1000);
    FAN_LEFT_DISABLE; 
    FAN_RIGHT_ENABLE; 
    HAL_Delay(1000);
    FAN_RIGHT_DISABLE; 
    PUMP_LEFT_ENABLE; 
    HAL_Delay(1000);
    PUMP_LEFT_DISABLE; 
    PUMP_RIGHT_ENABLE;
    HAL_Delay(1000); 
    PUMP_RIGHT_DISABLE;
    MC_LEFT_ENABLE; 
    HAL_Delay(1000);
    MC_LEFT_DISABLE;
    MC_RIGHT_ENABLE; 
    HAL_Delay(1000);
    MC_RIGHT_DISABLE; 
    TELEMETRY_ENABLE; 
    HAL_Delay(1000);
    TELEMETRY_DISABLE; 
    FAN_BATT_ENABLE; 
    HAL_Delay(1000);
    FAN_BATT_DISABLE; 
    return pdFALSE;
}
static const CLI_Command_Definition_t testOuputCommandDefinition =
{
    "testOuput",
    "testOuput:\r\n  Cycle through each of the outputs in 1s intervals.\r\n",
    testOuput,
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
    if (FreeRTOS_CLIRegisterCommand(&setBusVoltageCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&setChannelCurrentCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&testOuputCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    return HAL_OK;
}
