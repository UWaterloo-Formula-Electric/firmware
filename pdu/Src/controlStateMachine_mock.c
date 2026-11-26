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
#include <limits.h>
#include <math.h>
#include <stdlib.h>

extern volatile uint8_t acc_fan_command_override;

static bool parseCliUnsigned(const char *parameter, unsigned int *value)
{
    char *end;
    unsigned long parsed;

    if (parameter == NULL || value == NULL || parameter[0] == '-') {
        return false;
    }

    parsed = strtoul(parameter, &end, 10);
    if (end == parameter || (*end != '\0' && *end != ' ') ||
        parsed > UINT_MAX) {
        return false;
    }

    *value = (unsigned int)parsed;
    return true;
}

static bool parseCliInt(const char *parameter, int *value)
{
    char *end;
    long parsed;

    if (parameter == NULL || value == NULL) {
        return false;
    }

    parsed = strtol(parameter, &end, 10);
    if (end == parameter || (*end != '\0' && *end != ' ') ||
        parsed < INT_MIN || parsed > INT_MAX) {
        return false;
    }

    *value = (int)parsed;
    return true;
}

static bool parseCliFloat(const char *parameter, float *value)
{
    char *end;
    float parsed;

    if (parameter == NULL || value == NULL) {
        return false;
    }

    parsed = strtof(parameter, &end);
    if (end == parameter || (*end != '\0' && *end != ' ') ||
        !isfinite(parsed)) {
        return false;
    }

    *value = parsed;
    return true;
}

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

typedef struct {
    const char *name;
    const char *how;
    const char *expected;
} PDU_Test_Case_t;

typedef enum {
    PDU_TEST_NOT_RECORDED = 0,
    PDU_TEST_PASS,
    PDU_TEST_FAIL,
    PDU_TEST_BLOCKED
} PDU_Test_Result_t;

static const PDU_Test_Case_t pduTestCases[] = {
    {
        "Build and software tests",
        "Build with make pdu. Run make test TEST_TARGET=pdu. Save output.",
        "Firmware builds. Unit-test results are recorded separately."
    },
    {
        "Review what changed",
        "Compare revision schematics and change lists with the previous board.",
        "Every changed circuit has a test setup and acceptance limit."
    },
    {
        "Unpowered inspection",
        "Inspect parts, connectors, soldering, resistance, and continuity.",
        "No defects or shorts. Readings meet electrical-team limits."
    },
    {
        "First power-on and boot",
        "Isolate vehicle loads. Use current-limited supply. Flash and power-cycle.",
        "Repeatable boot with no excess current, heat, reset, or init error."
    },
    {
        "UART command check",
        "Use PDU UART settings. Try help, state, getBusMeas, and getChannels.",
        "Commands respond and output is readable."
    },
    {
        "CAN communication check",
        "Use CAN analyzer and termination. Capture/decode traffic with team DBC.",
        "Expected frames/data appear with no persistent CAN errors."
    },
    {
        "Bus voltage/current measurement",
        "Apply approved supply/load points. Compare getBusMeas and busRaw.",
        "Readings track calibrated instruments within approved tolerance."
    },
    {
        "Output-to-connector mapping",
        "Confirm indices in schematic. Use dummy loads; switch one at a time.",
        "Only intended output energizes and turns off; others stay unchanged."
    },
    {
        "Output-current sensing",
        "Use known dummy loads. Compare getChannels and adcRaw to meter.",
        "Correct channel reports current and meets approved tolerance."
    },
    {
        "Grouped controls and status",
        "Test board, controlPumps, controlFans, and auxEnable with fixture.",
        "Physical output and CAN status agree."
    },
    {
        "Changed input/protection circuits",
        "Use only approved simulator/fixture and hardware-owner procedure.",
        "Input/protection behaves correctly; no unapproved shorts or overloads."
    },
    {
        "Reset and shutdown",
        "Return outputs to safe state. Reset with loads isolated. Save logs.",
        "Startup is repeatable and all evidence is saved."
    }
};

#define PDU_TEST_COUNT (sizeof(pduTestCases) / sizeof(pduTestCases[0]))

static PDU_Test_Result_t pduTestResults[PDU_TEST_COUNT];
static uint32_t pduTestIndex;

static const char *pduTestResultString(PDU_Test_Result_t result)
{
    switch (result) {
        case PDU_TEST_PASS:
            return "PASS";
        case PDU_TEST_FAIL:
            return "FAIL";
        case PDU_TEST_BLOCKED:
            return "BLOCKED";
        default:
            return "NOT_RECORDED";
    }
}

BaseType_t pduTestCommand(char *writeBuffer, size_t writeBufferLength,
                          const char *commandString)
{
    BaseType_t paramLen;
    const char *action;
    const char *resultParam;
    PDU_Test_Case_t test;
    uint8_t state;

    if (pduTestIndex >= PDU_TEST_COUNT) {
        pduTestIndex = PDU_TEST_COUNT - 1U;
    }

    action = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    test = pduTestCases[pduTestIndex];

    if (action == NULL) {
        COMMAND_OUTPUT("Test %lu/%lu: %s\r\n",
                       (unsigned long)(pduTestIndex + 1U),
                       (unsigned long)PDU_TEST_COUNT,
                       test.name);
        return pdFALSE;
    }

    if (paramLen == 3 && STR_EQ(action, "how", paramLen)) {
        COMMAND_OUTPUT("How: %s\r\n", test.how);
        return pdFALSE;
    }

    if (paramLen == 6 && STR_EQ(action, "expect", paramLen)) {
        COMMAND_OUTPUT("Expect: %s\r\n", test.expected);
        return pdFALSE;
    }

    if (paramLen == 4 && STR_EQ(action, "next", paramLen)) {
        if (pduTestResults[pduTestIndex] == PDU_TEST_NOT_RECORDED) {
            COMMAND_OUTPUT("Record PASS, FAIL, or BLOCKED first.\r\n");
            return pdFALSE;
        }
        if (pduTestIndex + 1U < PDU_TEST_COUNT) {
            pduTestIndex++;
            test = pduTestCases[pduTestIndex];
            COMMAND_OUTPUT("Test %lu/%lu: %s\r\n",
                           (unsigned long)(pduTestIndex + 1U),
                           (unsigned long)PDU_TEST_COUNT,
                           test.name);
        } else {
            COMMAND_OUTPUT("Checklist complete. Use pdutest summary.\r\n");
        }
        return pdFALSE;
    }

    if (paramLen == 6 && STR_EQ(action, "result", paramLen)) {
        resultParam = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);
        if (resultParam == NULL) {
            COMMAND_OUTPUT("Usage: pdutest result PASS|FAIL|BLOCKED\r\n");
            return pdFALSE;
        }

        if (paramLen == 4 && STR_EQ(resultParam, "PASS", paramLen)) {
            pduTestResults[pduTestIndex] = PDU_TEST_PASS;
        } else if (paramLen == 4 && STR_EQ(resultParam, "FAIL", paramLen)) {
            pduTestResults[pduTestIndex] = PDU_TEST_FAIL;
        } else if (paramLen == 7 && STR_EQ(resultParam, "BLOCKED", paramLen)) {
            pduTestResults[pduTestIndex] = PDU_TEST_BLOCKED;
        } else {
            COMMAND_OUTPUT("Usage: pdutest result PASS|FAIL|BLOCKED\r\n");
            return pdFALSE;
        }

        COMMAND_OUTPUT("Test %lu: %s\r\n",
                       (unsigned long)(pduTestIndex + 1U),
                       pduTestResultString(pduTestResults[pduTestIndex]));
        return pdFALSE;
    }

    if (paramLen == 7 && STR_EQ(action, "summary", paramLen)) {
        size_t offset = (size_t)snprintf(writeBuffer, writeBufferLength,
                                         "PDU checklist:");
        for (uint32_t i = 0; i < PDU_TEST_COUNT; i++) {
            char status = '-';
            if (pduTestResults[i] == PDU_TEST_PASS) {
                status = 'P';
            } else if (pduTestResults[i] == PDU_TEST_FAIL) {
                status = 'F';
            } else if (pduTestResults[i] == PDU_TEST_BLOCKED) {
                status = 'B';
            }
            if (offset < writeBufferLength) {
                int written = snprintf(&writeBuffer[offset],
                                       writeBufferLength - offset,
                                       " %lu:%c",
                                       (unsigned long)(i + 1U),
                                       status);
                if (written > 0) {
                    offset += (size_t)written;
                }
            }
        }
        return pdFALSE;
    }

    if (paramLen == 4 && STR_EQ(action, "auto", paramLen)) {
        bool adc1InRange = true;
        bool adc3InRange = true;
        bool convertedValuesFinite = true;

        state = fsmGetState(&mainFsmHandle);
        for (size_t i = 0; i < NUM_ADC1_CHANNELS; i++) {
            if (ADC1_Buffer[i] > 4095U) {
                adc1InRange = false;
            }
            if (!isfinite(busResults.busMeas_a[i])) {
                convertedValuesFinite = false;
            }
        }
        for (size_t i = 0; i < NUM_PDU_CHANNELS; i++) {
            if (rawADC3Buffer[i] > 4095U) {
                adc3InRange = false;
            }
            if (!isfinite(channelCurrents.meas_a[i])) {
                convertedValuesFinite = false;
            }
        }

        COMMAND_OUTPUT("Auto: FSM=%s ADC1=%s ADC3=%s values=%s\r\n"
                       "Sanity only; not accuracy/freshness.\r\n",
                       state < STATE_ANY ? "PASS" : "FAIL",
                       adc1InRange ? "PASS" : "FAIL",
                       adc3InRange ? "PASS" : "FAIL",
                       convertedValuesFinite ? "PASS" : "FAIL");
        return pdFALSE;
    }

    COMMAND_OUTPUT("Usage: pdutest [how|expect|result PASS|FAIL|BLOCKED|next|summary|auto]\r\n");
    return pdFALSE;
}

static const CLI_Command_Definition_t pduTestCommandDefinition =
{
    "pdutest",
    "pdutest [how|expect|result PASS|FAIL|BLOCKED|next|summary|auto]:\r\n Guided PDU checklist and safe diagnostics\r\n",
    pduTestCommand,
    -1,
};

BaseType_t setChannelCurrent(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    int channelIdx;
    float current;

    const char *idxParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    const char *currentParam = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);

    if (!parseCliInt(idxParam, &channelIdx)) {
        COMMAND_OUTPUT("Usage: channelCurrent <channel> <current>\r\n");
        return pdFALSE;
    }

    if ((channelIdx < 0) || (channelIdx >= NUM_PDU_CHANNELS)) {
        COMMAND_OUTPUT("channelIdx must be between 0 and %d\n", NUM_PDU_CHANNELS - 1);
        return pdFALSE;
    }

    if (!parseCliFloat(currentParam, &current)) {
        COMMAND_OUTPUT("Invalid current value\r\n");
        return pdFALSE;
    }

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

    if (!parseCliFloat(voltageParam, &voltage) || voltage < 0.0f ||
        voltage > (4095.0f / ADC1_TO_VOLTS_DIVIDER)) {
        COMMAND_OUTPUT("Voltage must be finite and fit the 12-bit ADC range\r\n");
        return pdFALSE;
    }

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
    BaseType_t boardParamLen;
    BaseType_t onOffParamLen;
    const char *onOffParam = FreeRTOS_CLIGetParameter(commandString, 2, &onOffParamLen);
    const char *boardParam = FreeRTOS_CLIGetParameter(commandString, 1, &boardParamLen);

    bool onOff = false;
    if (onOffParam == NULL || boardParam == NULL) {
        COMMAND_OUTPUT("Usage: board <BMU|VCU|MOTEC|ALL> <on|off>\r\n");
        return pdFALSE;
    }

    if (onOffParamLen == 2 && STR_EQ(onOffParam, "on", onOffParamLen)) {
        onOff = true;
    } else if (onOffParamLen == 3 && STR_EQ(onOffParam, "off", onOffParamLen)) {
        onOff = false;
    } else {
        COMMAND_OUTPUT("Unknown parameter\n");
        return pdFALSE;
    }

    if (boardParamLen == 3 && STR_EQ(boardParam, "BMU", boardParamLen)) {
        COMMAND_OUTPUT("Turning BMU %s\n", onOff?"on":"off");
        if (onOff) {
            BMU_EN;
            StatusPowerBMU = StatusPowerBMU_CHANNEL_ON;
        } else {
            BMU_DISABLE;
            StatusPowerBMU = StatusPowerBMU_CHANNEL_OFF;
        }
    } else if (boardParamLen == 3 && STR_EQ(boardParam, "VCU", boardParamLen)) {
        COMMAND_OUTPUT("Turning VCU %s\n", onOff?"on":"off");
        if (onOff) {
            VCU_EN;
            StatusPowerVCU = StatusPowerVCU_CHANNEL_ON;
        } else {
            VCU_DISABLE;
            StatusPowerVCU = StatusPowerVCU_CHANNEL_OFF;
        }
    } else if (boardParamLen == 5 && STR_EQ(boardParam, "MOTEC", boardParamLen)) {
        COMMAND_OUTPUT("Turning MOTEC %s\n", onOff?"on":"off");
        if (onOff) {
            MOTEC_EN;
            StatusPowerMotec = StatusPowerMotec_CHANNEL_ON;
        } else {
            MOTEC_DISABLE;
            StatusPowerMotec = StatusPowerMotec_CHANNEL_OFF;
        }
    } else if (boardParamLen == 3 && STR_EQ(boardParam, "ALL", boardParamLen)) {
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
    "board <BMU|VCU|MOTEC|ALL> <on|off>:\r\n  Turn on/off board\r\n",
    boardEnableCommand,
    2 /* Number of parameters */
};

BaseType_t channelEnableCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t onOffParamLen;
    BaseType_t channelParamLen;
    const char *onOffParam = FreeRTOS_CLIGetParameter(commandString, 2, &onOffParamLen);

    bool onOff = false;
    if (onOffParam != NULL && onOffParamLen == 2 &&
        STR_EQ(onOffParam, "on", onOffParamLen)) {
        onOff = true;
    } else if (onOffParam != NULL && onOffParamLen == 3 &&
               STR_EQ(onOffParam, "off", onOffParamLen)) {
        onOff = false;
    } else {
        COMMAND_OUTPUT("Usage: channel <0-13> <on|off>\r\n");
        return pdFALSE;
    }

    unsigned int channel;
    const char *selectionParam = FreeRTOS_CLIGetParameter(commandString, 1, &channelParamLen);
    if (!parseCliUnsigned(selectionParam, &channel) ||
        channel >= NUM_PDU_CHANNELS) {
        COMMAND_OUTPUT("Channel must be between 0 and %d\r\n", NUM_PDU_CHANNELS - 1);
        return pdFALSE;
    }

    toggleChannel((uint8_t)channel, onOff);

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

    if (param != NULL && paramLen == 6 && STR_EQ(param, "enable", paramLen)) {
        fsmSendEventISR(&mainFsmHandle, EV_EM_Enable);
    } else if (param != NULL && paramLen == 7 && STR_EQ(param, "disable", paramLen)) {
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
    RADIATOR_1_EN;
    HAL_Delay(3000);
    RADIATOR_1_DISABLE;
    ACC_FANS_1_EN;
    HAL_Delay(3000);
    ACC_FANS_1_DISABLE;
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
    RADIATOR_2_EN;
    HAL_Delay(3000);
    RADIATOR_2_DISABLE;
    ACC_FANS_3_EN;
    HAL_Delay(3000);
    ACC_FANS_3_DISABLE;
    LAP_BEACON_EN;
    HAL_Delay(3000);
    LAP_BEACON_DISABLE;

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

    if (!parseCliUnsigned(selectionParam, &selection) || selection > 3U) {
        COMMAND_OUTPUT("Usage: controlFans <0|1|2|3>\r\n");
        return pdFALSE;
    }
    
    switch (selection)
    {
        case 0:
            DEBUG_PRINT("Turning all fans off!\r\n");
            ACC_FANS_1_DISABLE;
            ACC_FANS_2_DISABLE;
            ACC_FANS_3_DISABLE;
            RADIATOR_1_DISABLE;
            RADIATOR_2_DISABLE;
            acc_fan_command_override = 0;
            StatusPowerAccFan1 = StatusPowerAccFan1_CHANNEL_OFF;
            StatusPowerAccFan2 = StatusPowerAccFan2_CHANNEL_OFF;
            StatusPowerAccFan3 = StatusPowerAccFan3_CHANNEL_OFF;
            StatusPowerRadiator1 = StatusPowerRadiator1_CHANNEL_OFF;
            StatusPowerRadiator2 = StatusPowerRadiator2_CHANNEL_OFF;
            break;
        case 1:
            DEBUG_PRINT("Turning radiator fan on!\r\n");
            RADIATOR_1_EN;
            RADIATOR_2_EN;
            StatusPowerRadiator1 = StatusPowerRadiator1_CHANNEL_ON;
            StatusPowerRadiator2 = StatusPowerRadiator2_CHANNEL_ON;
            break;
        case 2:
            DEBUG_PRINT("Turning accumulator fans on!\r\n");
            acc_fan_command_override = 1;
            ACC_FANS_1_EN;
            ACC_FANS_2_EN;
            ACC_FANS_3_EN;
            StatusPowerAccFan1 = StatusPowerAccFan1_CHANNEL_ON;
            StatusPowerAccFan2 = StatusPowerAccFan2_CHANNEL_ON;
            StatusPowerAccFan3 = StatusPowerAccFan3_CHANNEL_ON;
            break;
        case 3:
            DEBUG_PRINT("Turning all fans on!\r\n");
            acc_fan_command_override = 1;
            ACC_FANS_1_EN;
            ACC_FANS_2_EN;
            ACC_FANS_3_EN;
            RADIATOR_1_EN;
            RADIATOR_2_EN;
            StatusPowerAccFan1 = StatusPowerAccFan1_CHANNEL_ON;
            StatusPowerAccFan2 = StatusPowerAccFan2_CHANNEL_ON;
            StatusPowerAccFan3 = StatusPowerAccFan3_CHANNEL_ON;
            StatusPowerRadiator1 = StatusPowerRadiator1_CHANNEL_ON;
            StatusPowerRadiator2 = StatusPowerRadiator2_CHANNEL_ON;
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

    if (!parseCliUnsigned(selectionParam, &selection) || selection > 3U) {
        COMMAND_OUTPUT("Usage: controlPumps <0|1|2|3>\r\n");
        return pdFALSE;
    }
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

    if (!parseCliUnsigned(selectionParam, &selection) || selection > 1U) {
        COMMAND_OUTPUT("Usage: invEnable <0|1>\r\n");
        return pdFALSE;
    }

    if (selection == 1U)
	{
		INVERTER_EN;
        StatusPowerInverter = StatusPowerInverter_CHANNEL_ON;
	}
	else
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
    int brakeValue;

    const char *selectionParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    if (!parseCliInt(selectionParam, &brakeValue)) {
        COMMAND_OUTPUT("Usage: fakeBrake <0-100>\r\n");
        return pdFALSE;
    }

    if (brakeValue > 100 || brakeValue < 0) {
        COMMAND_OUTPUT("Error: brake percent must be 0-100.\r\n");
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

    if (!parseCliUnsigned(channelParam, &channel) ||
        !parseCliUnsigned(stateParam, &power)) {
        COMMAND_OUTPUT("Usage: auxEnable <1-4> <0|1>\r\n");
        return pdFALSE;
    }

    // Validate channel input
    if (channel < 1 || channel > 4) {
        COMMAND_OUTPUT("Error: aux channel must be 1-4.\r\n");
        return pdFALSE;
    }

    if (power > 1U) {
        COMMAND_OUTPUT("Error: aux power must be 0 or 1.\r\n");
        return pdFALSE;
    }

    switch (channel)
    {
        case 1:
            if (power) { 
                TRANSPONDER_EN;
                StatusPowerTransponder = StatusPowerTransponder_CHANNEL_ON;  
            } else { 
                TRANSPONDER_DISABLE;
                StatusPowerTransponder = StatusPowerTransponder_CHANNEL_OFF;  
            }
            break;
        case 2:
            if (power) {
                RADIATOR_2_EN; 
                StatusPowerRadiator2 = StatusPowerRadiator2_CHANNEL_ON;  
            } else {
                RADIATOR_2_DISABLE;
                StatusPowerRadiator2 = StatusPowerRadiator2_CHANNEL_OFF;  
            }
            break;
        case 3:
            if (power) {
                ACC_FANS_3_EN; 
                StatusPowerAccFan3 = StatusPowerAccFan3_CHANNEL_ON;  
            } else {
                ACC_FANS_3_DISABLE;
                StatusPowerAccFan3 = StatusPowerAccFan3_CHANNEL_OFF;  
            }
            break;
        case 4:
            if (power) {
                LAP_BEACON_EN; 
                StatusPowerLapBeacon = StatusPowerLapBeacon_CHANNEL_ON;  
            } else {
                LAP_BEACON_DISABLE;
                StatusPowerLapBeacon = StatusPowerLapBeacon_CHANNEL_OFF;  
            }
            break;
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
    if (FreeRTOS_CLIRegisterCommand(&pduTestCommandDefinition) != pdPASS) {
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