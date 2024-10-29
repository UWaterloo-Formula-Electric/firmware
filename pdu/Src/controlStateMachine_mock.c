#include "controlStateMachine_mock.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "string.h"
#include "state_machine.h"
#include "FreeRTOS_CLI.h"
#include "sensors.h"
#include "canReceive.h"
#include "pdu_can.h"

extern uint32_t ADC_Buffer[NUM_PDU_CHANNELS];
extern volatile uint8_t acc_fan_command_override;

BaseType_t debugUartOverCan(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    COMMAND_OUTPUT("isUartOverCanEnabled: %u\n", isUartOverCanEnabled);

    return pdFALSE;
}
static const CLI_Command_Definition_t debugUartOverCanCommandDefinition =
{
    "isUartOverCanEnabled",
    "isUartOverCanEnabled help string",
    debugUartOverCan,
    0 /* Number of parameters */
};

BaseType_t getChannelCurrents(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    for (int i=0; i<NUM_PDU_CHANNELS; i++) {
        if (i == LV_Voltage) {
            DEBUG_PRINT("Bus Voltage: %f V\n", readBusVoltage());
            continue;
        } else if (i == LV_Current) {
            DEBUG_PRINT("Bus current: %f A\n", readBusCurrent());
            continue;
        }
        DEBUG_PRINT("Channel %s: %f A\n",channelNames[i], readCurrent(i));
    }

    return pdFALSE;
}
static const CLI_Command_Definition_t getChannelCurrentsCommandDefinition =
{
    "getChannels",
    "getChannels:\r\n Print all channels voltages/currents\r\n",
    getChannelCurrents,
    0,
};

BaseType_t getChannelsRaw(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    printRawADCVals();
    return pdFALSE;
}
static const CLI_Command_Definition_t getChannelsRawDefinition =
{
    "adcRaw",
    "adcRaw:\r\n Print all channels raw adc values\r\n",
    getChannelsRaw,
    0,
};

BaseType_t setChannelCurrent(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    int channelIdx;
    float current;

    const char *idxParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    const char *currentParam = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);

    sscanf(idxParam, "%u", &channelIdx);

    if ((channelIdx < 0) || (channelIdx >= NUM_PDU_CHANNELS)) {
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
    fsmSendEvent(&mainFsmHandle, EV_HV_CriticalFailure, portMAX_DELAY);
    return pdFALSE;
}
static const CLI_Command_Definition_t criticalCommandDefinition =
{
    "critical",
    "critical:\r\n  Generates a HV critical failure event\r\n",
    mockCritical,
    0 /* Number of parameters */
};


BaseType_t channelEnableCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * onOffParam = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);

    bool onOff = false;
    if (STR_EQ(onOffParam, "on", paramLen)) {
        onOff = true;
    } else if (STR_EQ(onOffParam, "off", paramLen)) {
        onOff = false;
    } else {
        COMMAND_OUTPUT("Unknown parameter\n");
        return pdFALSE;
    }

    const char * boardParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    if (STR_EQ(boardParam, "BMU", paramLen)) {
        COMMAND_OUTPUT("Turning BMU %s\n", onOff?"on":"off");
        if (onOff) {
            BMU_ENABLE;
        } else {
            BMU_DISABLE;
        }
    } else if (STR_EQ(boardParam, "DCU", paramLen)) {
        COMMAND_OUTPUT("Turning DCU %s\n", onOff?"on":"off");
        if (onOff) {
            DCU_ENABLE;
        } else {
            DCU_DISABLE;
        }
    } else if (STR_EQ(boardParam, "VCU_F7", paramLen)) {
        COMMAND_OUTPUT("Turning VCU_F7 %s\n", onOff?"on":"off");
        if (onOff) {
            VCU_ENABLE;
        } else {
            VCU_DISABLE;
        }
    } else if (STR_EQ(boardParam, "WSB", paramLen)) {
        COMMAND_OUTPUT("Turning WSB %s\n", onOff?"on":"off");
        if (onOff) {
            WSB_ENABLE;
        } else {
            WSB_DISABLE;
        }
    } else if (STR_EQ(boardParam, "ALL", paramLen)) {
        COMMAND_OUTPUT("Turning ALL %s\n", onOff?"on":"off");
        if (onOff) {
            BMU_ENABLE;
            DCU_ENABLE;
            VCU_ENABLE;
            WSB_ENABLE;
        } else {
            BMU_DISABLE;
            DCU_DISABLE;
            VCU_DISABLE;
            WSB_DISABLE;
        }
    } else {
        COMMAND_OUTPUT("Unknown parameter\n");
    }

    return pdFALSE;
}

static const CLI_Command_Definition_t channelEnableCommandDefinition =
{
    "board",
    "board <BMU|DCU|VCU_F7|WSB|ALL> <on|off>:\r\n  Turn on/off board\r\n",
    channelEnableCommand,
    2 /* Number of parameters */
};

BaseType_t mockEMEnableDisable(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    if (STR_EQ(param, "enable", paramLen)) {
        fsmSendEventISR(&mainFsmHandle, EV_EM_Enable);
    } else if (STR_EQ(param, "disable", paramLen)) {
        fsmSendEventISR(&mainFsmHandle, EV_EM_Disable);
    } else {
        COMMAND_OUTPUT("Unknown parameter\n");
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

BaseType_t printState(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    uint8_t main_index;
    main_index = fsmGetState(&mainFsmHandle);
    if (main_index >= 0 && main_index < STATE_ANY){
        COMMAND_OUTPUT("Main: %s\n", PDU_Main_States_String[main_index]);
    }else{
        COMMAND_OUTPUT("Error: main state index out of range. Main State Index: %u\n", main_index);
    }
    return pdFALSE;

}
static const CLI_Command_Definition_t printStateCommandDefinition =
{
    "state",
    "state:\r\n  Output current state of all state machines\r\n",
    printState,
    0 /* Number of parameters */
};
BaseType_t testOutput(char *writeBuffer, size_t writeBufferLength,
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
    MC_ENABLE; 
    HAL_Delay(1000);
    MC_DISABLE;
    HAL_Delay(1000);
    BRAKE_LIGHT_ENABLE;
    HAL_Delay(1000);
    BRAKE_LIGHT_DISABLE;
    AUX_ENABLE;
    HAL_Delay(1000);
    AUX_DISABLE;

    return pdFALSE;
}
static const CLI_Command_Definition_t testOutputCommandDefinition =
{
    "testOutput",
    "testOutput:\r\n  Cycle through each of the outputs in 1s intervals.\r\n",
    testOutput,
    0 /* Number of parameters */
};

BaseType_t printPowerStates(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{

    COMMAND_OUTPUT("States:\n DC present:%d, BMGR1: %d, BMGR2: %d, BMGR3:%d\n", CHECK_DC_DC_ON_PIN, CHECK_BMGR_GPIO1_PIN_STATE, CHECK_BMGR_GPIO2_PIN_STATE, CHECK_BMGR_GPIO3_PIN_STATE);
    return pdFALSE;
}
static const CLI_Command_Definition_t printPowerStatesCommandDefinition =
{
    "powerStates",
    "powerStates:\r\n  Output current states of LTC4110\r\n",
    printPowerStates,
    0 /* Number of parameters */
};

BaseType_t controlFans(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
	
    BaseType_t paramLen;
    unsigned int selection;

    const char *selectionParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(selectionParam, "%u", &selection);
    
    switch (selection)
    {
        case 0:
            DEBUG_PRINT("Turning all fans off!\r\n");
            ACC_FANS_DISABLE;
            RADIATOR_DISABLE;
            acc_fan_command_override = 0;
            break;
        case 1:
            DEBUG_PRINT("Turning radiator fan on!\r\n");
            RADIATOR_EN;
            break;
        case 2:
            DEBUG_PRINT("Turning accumulator fans on!\r\n");
            acc_fan_command_override = 1;
            ACC_FANS_EN;
            break;
        case 3:
            DEBUG_PRINT("Turning all fans on!\r\n");
            acc_fan_command_override = 1;
            ACC_FANS_EN;
            RADIATOR_EN;
            break;
        default:
            DEBUG_PRINT("Error: reached default case in controlFans!\r\n");
            break;
    }

    return pdFALSE;
}
static const CLI_Command_Definition_t controlFansCommandDefinition =
{
    "controlFans",
    "controlFans <0|1|2|3>:\r\n  Controls the power to the fans\r\n 0: Both off, 1: Radiator Fan On, 2: Acc. Fans On, 3: Both on\r\n",
    controlFans,
    1 /* Number of parameters */
};

BaseType_t controlPumps(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
	
    BaseType_t paramLen;
    unsigned int selection;

    const char *selectionParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(selectionParam, "%u", &selection);
    if (selection & 0x1)
	{
		PUMP_1_EN;
	}
	else
	{
		PUMP_1_DISABLE;
	}
	if (selection & 0x2)
	{
		PUMP_2_EN;
	}
	else
	{
		PUMP_2_DISABLE;
	}
    return pdFALSE;
}
static const CLI_Command_Definition_t controlPumpsCommandDefinition =
{
    /* TODO: Verify location of pump 1 and pump 2*/
    "controlPumps",
    "controlPumps <0|1|2|3>:\r\n  Controls the power to the pumps \r\n 0: Both off, 1: Right On, 2: Left On, 3: Both on\r\n",
    controlPumps,
    1 /* Number of parameters */
};

BaseType_t mcEnable(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    MC_ENABLE;
    return pdFALSE;
}
static const CLI_Command_Definition_t mcEnableCommandDefinition =
{
    "mcEnable",
    "mcEnable:\r\n Turn on motor controller\r\n",
    mcEnable,
    0 /* Number of parameters */
};

HAL_StatusTypeDef mockStateMachineInit()
{
    if (FreeRTOS_CLIRegisterCommand(&debugUartOverCanCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&criticalCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&emEnableDisableCommandDefinition) != pdPASS) {
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
    if (FreeRTOS_CLIRegisterCommand(&testOutputCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&channelEnableCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getChannelCurrentsCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getChannelsRawDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&printPowerStatesCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&controlPumpsCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&controlFansCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&mcEnableCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    return HAL_OK;
}
