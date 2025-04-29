#include "controlStateMachine_mock.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "string.h"
#include "state_machine.h"
#include "FreeRTOS_CLI.h"
#include "lvMeasure.h"
#include "loadSensor.h"
#include "canReceive.h"
#include "pdu_can.h"

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
    printChannelCurrent();
    return pdFALSE;
}

static const CLI_Command_Definition_t getChannelCurrentsCommandDefinition =
{
    "getChannels",
    "getChannels:\r\n Print all channels currents\r\n",
    getChannelCurrents,
    0,
};

BaseType_t getChannelsRaw(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    printRawChannelADCVals();
    return pdFALSE;
}

static const CLI_Command_Definition_t getChannelsRawDefinition =
{
    "adcRaw",
    "adcRaw:\r\n Print all channels raw adc values\r\n",
    getChannelsRaw,
    0,
};

BaseType_t getBusMeasurements(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    for (int i = 0; i < NUM_ADC1_CHANNELS; i++) {
        DEBUG_PRINT("%s: %f\r\n", diagnosticChannelNames[i], busResults.busMeas_a[i]);
    }

    return pdFALSE;
}

static const CLI_Command_Definition_t getBusMeasurementCommandDefinition =
{
    "getBusMeas",
    "getBusMeas:\r\n Print all voltages/currents on the bus\r\n",
    getBusMeasurements,
    0,
};

BaseType_t getBusRaw(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    printRawADCVals();
    return pdFALSE;
}

static const CLI_Command_Definition_t getBusRawDefinition =
{
    "busRaw",
    "busRaw:\r\n Print all the raw ADC values for the bus measurements\r\n",
    getBusRaw,
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

    // rawADC3Buffer[channelIdx] = current * ADC3_TO_AMPS_DIVIDER; // TODO: either read from CAN message or an array
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

    ADC1_Buffer[V_Main_Channel] = voltage * ADC1_TO_VOLTS_DIVIDER;
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

BaseType_t boardEnableCommand(char *writeBuffer, size_t writeBufferLength,
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
            BMU_EN;
            StatusPowerBMU = StatusPowerBMU_CHANNEL_ON;
        } else {
            BMU_DISABLE;
            StatusPowerBMU = StatusPowerBMU_CHANNEL_OFF;
        }
    } else if (STR_EQ(boardParam, "CDU", paramLen)) {
        COMMAND_OUTPUT("Turning DCU %s\n", onOff?"on":"off");
        if (onOff) {
            CDU_EN;
            StatusPowerCDU = StatusPowerCDU_CHANNEL_ON;
        } else {
            CDU_DISABLE;
            StatusPowerCDU = StatusPowerCDU_CHANNEL_OFF;
        }
    } else if (STR_EQ(boardParam, "TCU", paramLen)) {
        COMMAND_OUTPUT("Turning TCU %s\n", onOff?"on":"off");
        if (onOff) {
            TCU_EN;
            StatusPowerTCU = StatusPowerTCU_CHANNEL_ON;
        } else {
            TCU_DISABLE;
            StatusPowerTCU = StatusPowerTCU_CHANNEL_OFF;
        }
    } else if (STR_EQ(boardParam, "WSB", paramLen)) {
        COMMAND_OUTPUT("Turning ALL WSB %s\n", onOff?"on":"off");
        if (onOff) {
            WSB_EN;
            StatusPowerWSB = StatusPowerWSB_CHANNEL_ON;
        } else {
            WSB_DISABLE;
            StatusPowerWSB = StatusPowerWSB_CHANNEL_OFF;
        }
    } else if (STR_EQ(boardParam, "ALL", paramLen)) {
        COMMAND_OUTPUT("Turning ALL %s\n", onOff?"on":"off");
        if (onOff) {
            turnBoardsOn();
        } else {
            turnBoardsOff();
        }
    } else {
        COMMAND_OUTPUT("Unknown parameter\n");
    }

    if (sendCAN_PDU_ChannelStatus() != HAL_OK) {
        ERROR_PRINT("Failed to send pdu channel status CAN message\n");
    }

    return pdFALSE;
}

static const CLI_Command_Definition_t boardEnableCommandDefinition =
{
    "board",
    "board <BMU|CDU|TCU|WSB|ALL> <on|off>:\r\n  Turn on/off board\r\n",
    boardEnableCommand,
    2 /* Number of parameters */
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

    int channel;
    const char *selectionParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    sscanf(selectionParam, "%u", &channel);

    toggleChannel(channel, onOff);

    return pdFALSE;
}

static const CLI_Command_Definition_t channelEnableCommandDefinition =
{
    "channel",
    "channel <0-13> <on|off>:\r\n  Turn on/off channel\r\n",
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
    RADIATOR_EN;
    HAL_Delay(3000);
    RADIATOR_DISABLE;
    ACC_FANS_EN;
    HAL_Delay(3000);
    ACC_FANS_DISABLE;
    PUMP_1_EN; 
    HAL_Delay(3000);
    PUMP_1_DISABLE; 
    PUMP_2_EN;
    HAL_Delay(3000); 
    PUMP_2_DISABLE;
    INVERTER_EN; 
    HAL_Delay(3000);
    INVERTER_DISABLE;
    HAL_Delay(3000);
    BRAKE_LIGHT_ENABLE;
    HAL_Delay(3000);
    BRAKE_LIGHT_DISABLE;
    TRANSPONDER_EN;
    HAL_Delay(3000);
    TRANSPONDER_DISABLE;
    AUX_2_EN;
    HAL_Delay(3000);
    AUX_2_DISABLE;
    AUX_3_EN;
    HAL_Delay(3000);
    AUX_3_DISABLE;
    AUX_4_EN;
    HAL_Delay(3000);
    AUX_4_DISABLE;

    return pdFALSE;
}
static const CLI_Command_Definition_t testOutputCommandDefinition =
{
    "testOutput",
    "testOutput:\r\n  Cycle through each of the outputs in 3s intervals.\r\n",
    testOutput,
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
            StatusPowerAccFan = StatusPowerAccFan_CHANNEL_OFF;
            StatusPowerRadiator = StatusPowerRadiator_CHANNEL_OFF; 
            break;
        case 1:
            DEBUG_PRINT("Turning radiator fan on!\r\n");
            RADIATOR_EN;
            StatusPowerRadiator = StatusPowerRadiator_CHANNEL_ON;
            break;
        case 2:
            DEBUG_PRINT("Turning accumulator fans on!\r\n");
            acc_fan_command_override = 1;
            ACC_FANS_EN;
            StatusPowerAccFan = StatusPowerAccFan_CHANNEL_ON;
            break;
        case 3:
            DEBUG_PRINT("Turning all fans on!\r\n");
            acc_fan_command_override = 1;
            ACC_FANS_EN;
            RADIATOR_EN;
            StatusPowerAccFan = StatusPowerAccFan_CHANNEL_ON;
            StatusPowerRadiator = StatusPowerRadiator_CHANNEL_ON;
            break;
        default:
            DEBUG_PRINT("Error: reached default case in controlFans!\r\n");
            break;
    }

    if (sendCAN_PDU_ChannelStatus() != HAL_OK) {
        ERROR_PRINT("Failed to send pdu channel status CAN message\n");
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
        StatusPowerCoolingPump1 = StatusPowerCoolingPump1_CHANNEL_ON; 
        DEBUG_PRINT("Pump 1 on\n");
    }
	else
	{
		PUMP_1_DISABLE;
        StatusPowerCoolingPump1 = StatusPowerCoolingPump1_CHANNEL_OFF; 
        DEBUG_PRINT("Pump 1 off\n");
    }
	if (selection & 0x2)
	{
		PUMP_2_EN;
        StatusPowerCoolingPump2 = StatusPowerCoolingPump2_CHANNEL_ON; 
        DEBUG_PRINT("Pump 2 on\n");
    }
	else
	{
		PUMP_2_DISABLE;
        StatusPowerCoolingPump2 = StatusPowerCoolingPump2_CHANNEL_OFF; 
        DEBUG_PRINT("Pump 2 off\n");
    }
    

    if (sendCAN_PDU_ChannelStatus() != HAL_OK) {
        ERROR_PRINT("Failed to send pdu channel status CAN message\n");
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

BaseType_t invEnable(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    unsigned int selection;

    const char *selectionParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(selectionParam, "%u", &selection);
    if (selection == 1)
	{
		INVERTER_EN;
        StatusPowerInverter = StatusPowerInverter_CHANNEL_ON;
	}
	else if (selection == 0)
	{
		INVERTER_DISABLE;
        StatusPowerInverter = StatusPowerInverter_CHANNEL_OFF;
	}

    if (sendCAN_PDU_ChannelStatus() != HAL_OK) {
        ERROR_PRINT("Failed to send pdu channel status CAN message\n");
    }
    return pdFALSE;
}
static const CLI_Command_Definition_t invEnableCommandDefinition =
{
    "invEnable",
    "invEnable <0|1>:\r\n Turn off/on motor controller\r\n",
    invEnable,
    1 /* Number of parameters */
};

BaseType_t fakeBrake(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
	
    BaseType_t paramLen;
    unsigned int brakeValue;

    const char *selectionParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(selectionParam, "%u", &brakeValue);

    // Validate input
    if (brakeValue > 100 || brakeValue < 0){
        DEBUG_PRINT("Error: invalid brake percent. Acceptable range is 0-100!\r\n");
        return pdFALSE;
    }
    
    BrakePercent = brakeValue;

    return pdFALSE;
}

static const CLI_Command_Definition_t fakeBrakeCommandDefinition =
{
    "fakeBrake",
    "fakeBrake <brakeValue>:\r\n  Mock value for BrakePercent \r\n",
    fakeBrake,
    1 /* Number of parameters */
};

BaseType_t auxEnable(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    unsigned int channel, power;

    const char *channelParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    const char *stateParam = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);

    sscanf(channelParam, "%u", &channel);
    sscanf(stateParam, "%u", &power);

    // Validate channel input
    if (channel < 1 || channel > 4) {
        DEBUG_PRINT("Error: Invalid channel in auxEnable!\r\n");
        return pdFALSE;
    }

    switch (channel)
    {
        case 1:
            if (power) { 
                TRANSPONDER_EN;
                StatusPowerAux1 = StatusPowerAux1_CHANNEL_ON;  
            } else { 
                TRANSPONDER_DISABLE;
                StatusPowerAux1 = StatusPowerAux1_CHANNEL_OFF;  
            }
        case 2:
            if (power) {
                AUX_2_EN; 
                StatusPowerAux2 = StatusPowerAux2_CHANNEL_ON;  
            } else {
                AUX_2_DISABLE;
                StatusPowerAux2 = StatusPowerAux2_CHANNEL_OFF;  
            }
        case 3:
            if (power) {
                AUX_3_EN; 
                StatusPowerAux3 = StatusPowerAux3_CHANNEL_ON;  
            } else {
                AUX_3_DISABLE;
                StatusPowerAux3 = StatusPowerAux3_CHANNEL_OFF;  
            }
        case 4:
            if (power) {
                AUX_4_EN; 
                StatusPowerAux4 = StatusPowerAux4_CHANNEL_ON;  
            } else {
                AUX_4_DISABLE;
                StatusPowerAux4 = StatusPowerAux4_CHANNEL_OFF;  
            }
        default:
            DEBUG_PRINT("Error: reached default case in auxEnable!\r\n");
            break;
    }

    if (sendCAN_PDU_ChannelStatus() != HAL_OK) {
        ERROR_PRINT("Failed to send pdu channel status CAN message\n");
    }

    return pdFALSE;
}

static const CLI_Command_Definition_t auxEnableCommandDefinition =
{
    "auxEnable",
    "auxEnable <channel> <0|1>:\r\n Turn auxiliary <channel> off/on\r\n",
    auxEnable,
    2 /* Number of parameters */
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
    if (FreeRTOS_CLIRegisterCommand(&boardEnableCommandDefinition) != pdPASS) {
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
    if (FreeRTOS_CLIRegisterCommand(&controlPumpsCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&controlFansCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&invEnableCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&fakeBrakeCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&auxEnableCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getBusRawDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getBusMeasurementCommandDefinition) != pdPASS){
        return HAL_ERROR;
    }

    return HAL_OK;
}