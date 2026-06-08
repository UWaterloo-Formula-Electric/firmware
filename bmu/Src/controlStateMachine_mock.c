/**
  *****************************************************************************
  * @file    controlStateMachine_mock.c
  * @author  Richard Matthews
  * @brief   Module containing CLI commands for the BMU
  * @details This file contains implementations of all the BMU specific CLI
  * commands availabe on the BMU command line interface (CLI).
  ******************************************************************************
  */

#include "controlStateMachine_mock.h"
#include "debug.h"
#include "string.h"
#include "state_machine.h"
#include "FreeRTOS_CLI.h"
#include "task.h"
#include "cmsis_os.h"
#include "prechargeDischarge.h"
#include "bmu_can.h"
#include "bmu_dtc.h"
#include "controlStateMachine.h"
#include "testData.h"
#include "filters.h"
#include "sense.h"
#include "chargerControl.h"
#include "batteries.h"
#include "faultMonitor.h"
#include "ltc_chip.h"
#include "ltc_common.h"
#include "ltc_chip_interface.h"
#include "imdDriver.h"


extern bool HITL_Precharge_Mode;
extern float HITL_VPACK;
extern uint32_t brakeAndHallAdcVals[2];
extern float adjustedCellIR;

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

BaseType_t getBrakePressure(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    COMMAND_OUTPUT("Brake %f %%, (adcVal: %lu)\n",
                   ((float)brakeAndHallAdcVals[BRAKE_HALL_ADC_CHANNEL_BRAKE]) / BRAKE_ADC_DIVIDER,
                   brakeAndHallAdcVals[BRAKE_HALL_ADC_CHANNEL_BRAKE]);

    return pdFALSE;
}
static const CLI_Command_Definition_t getBrakePressureCommandDefinition =
{
    "getBrake",
    "getBrake:\r\n Get brake pressure\r\n",
    getBrakePressure,
    0 /* Number of parameters */
};

static void warnImdErrorThresholdBelowRulesMinimum(char *writeBuffer, size_t writeBufferLength,
                                                   uint32_t thresholdKohm)
{
    if (thresholdKohm < IMD_RULES_ERROR_THRESHOLD_MIN_KOHM) {
        COMMAND_OUTPUT("WARNING: Rules specify 500 ohms/V; at %uV the minimum is %u kOhm\n",
                       IMD_RULES_REFERENCE_PACK_VOLTAGE,
                       IMD_RULES_ERROR_THRESHOLD_MIN_KOHM);
    }
}

BaseType_t setImdErrorThreshold(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    uint32_t thresholdKohm;
    const char *param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    if (param == NULL || sscanf(param, "%lu", &thresholdKohm) != 1) {
        COMMAND_OUTPUT("Usage: setImdErrorThreshold <kOhm>\n");
        return pdFALSE;
    }

    if (thresholdKohm < IMD_ISOLATION_THRESHOLD_ERROR_MIN_KOHM ||
        thresholdKohm > IMD_ISOLATION_THRESHOLD_ERROR_MAX_KOHM) {
        COMMAND_OUTPUT("IMD error threshold must be %u-%u kOhm\n",
                       IMD_ISOLATION_THRESHOLD_ERROR_MIN_KOHM,
                       IMD_ISOLATION_THRESHOLD_ERROR_MAX_KOHM);
        return pdFALSE;
    }

    warnImdErrorThresholdBelowRulesMinimum(writeBuffer, writeBufferLength, thresholdKohm);

    if (imdSetIsolationThresholdError((uint16_t)thresholdKohm) != HAL_OK) {
        COMMAND_OUTPUT("Failed to send IMD error threshold request\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("Requested IMD error threshold set to %lu kOhm\n", thresholdKohm);
    return pdFALSE;
}
static const CLI_Command_Definition_t setImdErrorThresholdCommandDefinition =
{
    "setImdErrorThreshold",
    "setImdErrorThreshold <kOhm>:\r\n Set IMD isolation error threshold\r\n",
    setImdErrorThreshold,
    1 /* Number of parameters */
};

BaseType_t getImdErrorThreshold(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    uint16_t thresholdKohm;

    if (imdRequestIsolationThresholdError() != HAL_OK) {
        COMMAND_OUTPUT("Failed to send IMD error threshold read request\n");
        return pdFALSE;
    }

    vTaskDelay(pdMS_TO_TICKS(IMD_CLI_RESPONSE_WAIT_MS));

    if (!imdGetIsolationThresholdError(&thresholdKohm)) {
        COMMAND_OUTPUT("No valid IMD error threshold response received: %d\n", thresholdKohm);
        return pdFALSE;
    }

    COMMAND_OUTPUT("IMD error threshold: %u kOhm\n", thresholdKohm);
    warnImdErrorThresholdBelowRulesMinimum(writeBuffer, writeBufferLength, thresholdKohm);
    return pdFALSE;
}
static const CLI_Command_Definition_t getImdErrorThresholdCommandDefinition =
{
    "getImdErrorThreshold",
    "getImdErrorThreshold:\r\n Read IMD isolation error threshold\r\n",
    getImdErrorThreshold,
    0 /* Number of parameters */
};

BaseType_t setBrakePressure(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    float brakePressure;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(param, "%f", &brakePressure);
    brakeAndHallAdcVals[BRAKE_HALL_ADC_CHANNEL_BRAKE] = brakePressure * BRAKE_ADC_DIVIDER;
    COMMAND_OUTPUT("Setting brake to %f %%, (adcVal: %lu)\n", brakePressure, brakeAndHallAdcVals[BRAKE_HALL_ADC_CHANNEL_BRAKE]);

    return pdFALSE;
}
static const CLI_Command_Definition_t brakePressureCommandDefinition =
{
    "brake",
    "brake <val>:\r\n Set brake pressure to val \r\n",
    setBrakePressure,
    1 /* Number of parameters */
};

float output[DATA_LENGTH];
BaseType_t testLowPassFilter(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    const int BLOCK_SIZE = 20;
    float *samplePointer = data;
    float *outputPointer = output;

    filtersInit();

    uint32_t startTime = getRunTimeCounterValue();
    for (int i=0; i<DATA_LENGTH / BLOCK_SIZE; i++) {
        samplePointer = &(data[i*BLOCK_SIZE]);
        outputPointer = &(output[i*BLOCK_SIZE]);
        lowPassFilter(samplePointer, BLOCK_SIZE, outputPointer);
    }
    uint32_t elapsedTime = getRunTimeCounterValue() - startTime;

    DEBUG_PRINT("Filtered data length %d in %lu us\n", DATA_LENGTH, elapsedTime*50);
    DEBUG_PRINT("outputData = [\n");
    vTaskDelay(10);
    for (int j=0; j<DATA_LENGTH; j++) {
        DEBUG_PRINT("%f,\n", output[j]);
        vTaskDelay(2);
    }
    return pdFALSE;
}
static const CLI_Command_Definition_t testLowPassFilterCommandDefinition =
{
    "lowPass",
    "lowPass:\r\n Print output of low pass filter\r\n",
    testLowPassFilter,
    0 /* Number of parameters */
};

BaseType_t printBattInfo(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{

    static int cellIdx = -6;

    float IBus, VBus, VBatt, packVoltage;

    getIBus(&IBus);
    getVBus(&VBus);
    getVBatt(&VBatt);

    if (cellIdx == -6) {
        COMMAND_OUTPUT("IBUS\tVBUS\tVBATT\r\n");
        cellIdx = -5;
        return pdTRUE;
    } else if (cellIdx == -5) {
        COMMAND_OUTPUT("%f\t%f\t%f\r\n\n", IBus, VBus, VBatt);
        cellIdx = -4;
        return pdTRUE;
    } else if (cellIdx == -4) {
        COMMAND_OUTPUT("MinVoltage\tMaxVoltage\tMinTemp\tMaxTemp\tPackVoltage\r\n");
        cellIdx = -3;
        return pdTRUE;
    } else if (cellIdx == -3) {
		getPackVoltage(&packVoltage);
        COMMAND_OUTPUT("%f\t%f\t%f\t%f\t%f\r\n\n", VoltageCellMin, VoltageCellMax, TempCellMin, TempCellMax, packVoltage);
        cellIdx = -2;
        return pdTRUE;
    } else if (cellIdx == -2) {
    	COMMAND_OUTPUT("*Note Temp is not related to a specific cell number\r\n\n");
    	cellIdx = -1;
    	return pdTRUE;
	} else if (cellIdx == -1) {
        COMMAND_OUTPUT("Index\tCell Voltage(V)\tTemp Channel(degC)\r\n");
        cellIdx = 0;
        return pdTRUE;
    }
    // Note that the temperature channels are not correlated with the voltage cell
	else if(cellIdx >= NUM_VOLTAGE_CELLS && cellIdx < NUM_TEMP_CELLS){
        COMMAND_OUTPUT("%d\t(N/A)\t%f\r\n", cellIdx+1, TempChannel[cellIdx]);
    } else if(cellIdx >= 0 && cellIdx < NUM_VOLTAGE_CELLS && cellIdx < NUM_TEMP_CELLS) {
        COMMAND_OUTPUT("%d\t%f\t%f\r\n", cellIdx+1, VoltageCell[cellIdx], TempChannel[cellIdx]);
    }
	else {
		// Do nothing
	}
	++cellIdx;
    if (cellIdx >= NUM_VOLTAGE_CELLS && cellIdx >= NUM_TEMP_CELLS) {
        cellIdx = -6;
        return pdFALSE;
    } else {
        vTaskDelay(1); // Hack to avoid overflowing our serial buffer
        return pdTRUE;
    }
}
static const CLI_Command_Definition_t printBattInfoCommandDefinition =
{
    "battInfo",
    "battInfo:\r\n Print info about battery pack\r\n",
    printBattInfo,
    0 /* Number of parameters */
};

BaseType_t setCellVoltage(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    int cellIdx;

    const char *idxParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    const char *voltageParam = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);

    sscanf(idxParam, "%u", &cellIdx);

    if (cellIdx < 0 || cellIdx >= NUM_VOLTAGE_CELLS) {
        COMMAND_OUTPUT("Cell Index must be between 0 and %d\n", NUM_VOLTAGE_CELLS);
        return pdFALSE;
    }

    sscanf(voltageParam, "%f", &VoltageCell[cellIdx]);
    COMMAND_OUTPUT("VoltageCell[%d] = %fV\n", cellIdx, VoltageCell[cellIdx]);
    if( VoltageCell[cellIdx] > 4.2 || VoltageCell[cellIdx] < 2.5 ) 
    { 
        // TODO: as of 29-04-2026, the pack suffered a lot of EMI issues and would fault right away at EM since
        // We couldn't talk to pack. We by passed this (increased redcar error counter), but it should be fixed
        // Revert once it is fixed.
        TSSI_GREEN_OFF;
        TSSI_RED_ON; 
        AMS_CONT_OPEN;
        sendDTC_FATAL_AMS_Failure();
        fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, pdMS_TO_TICKS(500));
    }
    return pdFALSE;
}
static const CLI_Command_Definition_t setCellVoltageCommandDefinition =
{
    "voltageCell",
    "voltageCell <idx> <voltage>:\r\n Set a cells voltage\r\n",
    setCellVoltage,
    2 /* Number of parameters */
};

BaseType_t setChannelTemp(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    int cellIdx;

    const char *idxParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    const char *tempParam = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);

    sscanf(idxParam, "%u", &cellIdx);

    if (cellIdx < 0 || cellIdx >= NUM_TEMP_CELLS) {
        COMMAND_OUTPUT("Cell Index must be between 0 and %d\n", NUM_TEMP_CELLS);
        return pdFALSE;
    }

    sscanf(tempParam, "%f", &TempChannel[cellIdx]);
    COMMAND_OUTPUT("TempChannel[%d] = %fdegC\n", cellIdx, TempChannel[cellIdx]);
    return pdFALSE;
}
static const CLI_Command_Definition_t setChannelTempCommandDefinition =
{
    "tempChannel",
    "tempChannel <idx> <temp>:\r\n Set a channels temperature\r\n",
    setChannelTemp,
    2 /* Number of parameters */
};

BaseType_t printHVMeasurements(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    float IBus, VBus, VBatt;
    getIBus(&IBus);
    getVBus(&VBus);
    getVBatt(&VBatt);

    COMMAND_OUTPUT("IShunt: %f\nVBus: %f\nVBatt: %f\n", IBus, VBus, VBatt);
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

    float VBatt;
    sscanf(param, "%f", &VBatt);
    COMMAND_OUTPUT("Setting VBatt %f\n", VBatt);

    cliSetVBatt(VBatt);

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

    float VBus;
    sscanf(param, "%f", &VBus);
    COMMAND_OUTPUT("Setting VBus %f\n", VBus);

    cliSetVBus(VBus);
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
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    float current;
    sscanf(param, "%f", &current);

    COMMAND_OUTPUT("setting bus current %f\n", current);

    cliSetIBus(current);

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

BaseType_t fakeHV_Toggle(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEventISR(&fsmHandle, EV_HV_Toggle);
    return pdFALSE;
}
static const CLI_Command_Definition_t hvToggleCommandDefinition =
{
    "hvToggle",
    "hvToggle:\r\n Send hv toggle event\r\n",
    fakeHV_Toggle,
    0 /* Number of parameters */
};

/* Names must stay in sync with BMU_Events_t in bmu/Inc/controlStateMachine.h (BMU FSM, not PDU). */
static const char *const bmu_event_names[] = {
    "EV_Init",
    "EV_HV_Toggle",
    "EV_Precharge_Finished",
    "EV_Discharge_Finished",
    "EV_PrechargeDischarge_Fail",
    "EV_HV_Fault",
    "EV_IMD_Ready",
    "EV_FaultMonitorReady",
    "EV_Enter_Charge_Mode",
    "EV_Charge_Start",
    "EV_Notification_Done",
    "EV_Charge_Error",
    "EV_Notification_Stop",
    "EV_Cockpit_BRB_Pressed",
    "EV_Cockpit_BRB_Unpressed",
    "EV_Balance_Start",
    "EV_Balance_Stop",
    "EV_ANY",
};

BaseType_t mockFsmEvent(char *writeBuffer, size_t writeBufferLength,
                        const char *commandString)
{
    (void)writeBuffer;
    (void)writeBufferLength;

    BaseType_t paramLen;
    const char *param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    if (param == NULL) {
        COMMAND_OUTPUT("mockFsmEvent: need <id> or list (BMU fsmHandle; see BMU_Events_t)\r\n");
        return pdFALSE;
    }

    if (paramLen == 4 && strncmp(param, "list", 4) == 0) {
        COMMAND_OUTPUT("BMU_Events_t (bmu/Inc/controlStateMachine.h):\r\n");
        for (uint32_t i = 0; i <= (uint32_t)EV_ANY; i++) {
            COMMAND_OUTPUT("  %2u  %s\r\n", (unsigned)i, bmu_event_names[i]);
        }
        return pdFALSE;
    }

    unsigned long id_ul;
    if (sscanf(param, "%lu", &id_ul) != 1) {
        COMMAND_OUTPUT("mockFsmEvent: invalid id (use decimal or \"list\")\r\n");
        return pdFALSE;
    }

    if (id_ul > (unsigned long)EV_ANY) {
        COMMAND_OUTPUT("mockFsmEvent: id must be 0..%u\r\n", (unsigned)EV_ANY);
        return pdFALSE;
    }

    const uint32_t id = (uint32_t)id_ul;
    if (fsmSendEvent(&fsmHandle, id, portMAX_DELAY) != HAL_OK) {
        COMMAND_OUTPUT("mockFsmEvent: fsmSendEvent failed for %s (%lu)\r\n",
                     bmu_event_names[id], id_ul);
        return pdFALSE;
    }

    COMMAND_OUTPUT("mockFsmEvent: sent %s (%lu)\r\n", bmu_event_names[id], id_ul);
    return pdFALSE;
}

static const CLI_Command_Definition_t mockFsmEventCommandDefinition =
{
    "mockFsmEvent",
    "mockFsmEvent <id>|list:\r\n Post BMU FSM event by id (BMU_Events_t). Use \"list\" for ids.\r\n",
    mockFsmEvent,
    1 /* Number of parameters */
};

BaseType_t fakeEnter_Charge_Mode(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEventISR(&fsmHandle, EV_Enter_Charge_Mode);
    return pdFALSE;
}
static const CLI_Command_Definition_t fakeEnterChargeModeCommandDefinition =
{
    "enterChargeMode",
    "enterChargeMode:\r\n Send EV_Enter_Charge_Mode event\r\n",
    fakeEnter_Charge_Mode,
    0 /* Number of parameters */
};

BaseType_t printState(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    uint8_t index;
    index = fsmGetState(&fsmHandle);
    if ( index >= 0 && index < STATE_ANY ){
        COMMAND_OUTPUT("State: %s\n", BMU_states_string[index]);
    } else {
        COMMAND_OUTPUT("Error: state index out of range. Index: %u\n", index);
    }
    return pdFALSE;
}
static const CLI_Command_Definition_t printStateCommandDefinition =
{
    "state",
    "state:\r\n  Output current state of state machine\r\n",
    printState,
    0 /* Number of parameters */
};

BaseType_t maxChargeCurrentCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    float current;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(param, "%f", &current);

    COMMAND_OUTPUT("setting max charge current %f\n", current);
    if (setMaxChargeCurrent(current) != HAL_OK) {
        ERROR_PRINT("Failed to set max charge current\n");
    }

    return pdFALSE;
}

static const CLI_Command_Definition_t maxChargeCurrentCommandDefinition =
{
    "maxChargeCurrent",
    "maxChargeCurrent <current>:\r\n  set the max current the charger will output\r\n",
    maxChargeCurrentCommand,
    1 /* Number of parameters */
};

BaseType_t startChargeCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEvent(&fsmHandle, EV_Charge_Start, portMAX_DELAY);
    return pdFALSE;
}
static const CLI_Command_Definition_t startChargeCommandDefinition =
{
    "startCharge",
    "startCharge:\r\n  start charging\r\n",
    startChargeCommand,
    0 /* Number of parameters */
};

BaseType_t stopChargeCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEvent(&fsmHandle, EV_Notification_Stop, portMAX_DELAY);
    return pdFALSE;
}
static const CLI_Command_Definition_t stopChargeCommandDefinition =
{
    "stopCharge",
    "stopCharge:\r\n  stop charging\r\n",
    stopChargeCommand,
    0 /* Number of parameters */
};

BaseType_t chargeCartHeartbeatMockCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    CAN_Msg_ChargeCart_Heartbeat_Callback();
    return pdFALSE;
}
static const CLI_Command_Definition_t chargeCartHeartbeatMockCommandDefinition =
{
    "ccHeartbeat",
    "ccHeartbeat:\r\n  mock charge cart heartbeat receive\r\n",
    chargeCartHeartbeatMockCommand,
    0 /* Number of parameters */
};

BaseType_t setPosCont(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    int contState;

    const char *idxParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(idxParam, "%u", &contState);
    switch (contState)
    {
        case 0:
            CONT_POS_OPEN;
        break;

        case 1:
            CONT_POS_CLOSE;
        break;

        default:
            COMMAND_OUTPUT("Cell Index must be between 0 and %d\n", NUM_VOLTAGE_CELLS);
            return pdFALSE;    
        break;
    }

    return pdFALSE;
}
static const CLI_Command_Definition_t setPosContCommandDefinition =
{
    "setPosCont",
    "setPosCont <state>:\r\n sets the state of the positive contactor, 1-> closed , 0-> open\r\n",
    setPosCont,
    1 /* Number of parameters */
};

BaseType_t setNegCont(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    int contState;

    const char *idxParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(idxParam, "%u", &contState);
    switch (contState)
    {
        case 0:
            CONT_NEG_OPEN;
        break;

        case 1:
            CONT_NEG_CLOSE;
        break;

        default:
            COMMAND_OUTPUT("Cell Index must be between 0 and %d\n", NUM_VOLTAGE_CELLS);
            return pdFALSE;    
        break;
    }

    return pdFALSE;
}
static const CLI_Command_Definition_t setNegContCommandDefinition =
{
    "setNegCont",
    "setNegCont <state>:\r\n sets the state of the negative contactor, 1-> closed , 0-> open\r\n",
    setNegCont,
    1 /* Number of parameters */
};

BaseType_t setPCDC(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    int contState;

    const char *idxParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(idxParam, "%u", &contState);
    switch (contState)
    {
        case 0:
            PCDC_DC;
        break;

        case 1:
            PCDC_PC;
        break;

        default:
            COMMAND_OUTPUT("Cell Index must be between 0 and %d\n", NUM_VOLTAGE_CELLS);
            return pdFALSE;    
        break;
    }

    return pdFALSE;
}
static const CLI_Command_Definition_t setPCDCCommandDefinition =
{
    "setPCDC",
    "setPCDC <state>:\r\n sets the state of the precharge / Discharge relay, 1-> precharge , 0-> dishcage\r\n",
    setPCDC,
    1 /* Number of parameters */
};

BaseType_t getStateBusHVSendPeriod(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    uint32_t current_period = cliGetStateBusHVSendPeriod();
    COMMAND_OUTPUT("The current state bus HV CAN send period: %lu\r\n", current_period);

    return pdFALSE;
}
static const CLI_Command_Definition_t getStateBusHVSendPeriodCommandDefinition =
{
    "getStateBusHVSendPeriod",
    "getStateBusHVSendPeriod:\r\n Get the state bus HV CAN send period\r\n",
    getStateBusHVSendPeriod,
    0 /* Number of parameters */
};

BaseType_t setStateBusHVSendPeriod(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    uint32_t period_ms;

    const char *idxParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    if (idxParam[0] == '-') {
        COMMAND_OUTPUT("The publishing time (in ms) must be greater than 0");
    } else {
        sscanf(idxParam, "%lu", &period_ms);
        cliSetStateBusHVSendPeriod(period_ms);
    }

    return pdFALSE;
}
static const CLI_Command_Definition_t setStateBusHVSendPeriodCommandDefinition =
{
    "setStateBusHVSendPeriod",
    "setStateBusHVSendPeriod <period>:\r\n  set the period/interval for sending state bus HV CAN messages\r\n",
    setStateBusHVSendPeriod,
    1 /* Number of parameters */
};

BaseType_t sendChargerCLICommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    DEBUG_PRINT("Sending charger command\n");
    if (sendChargerCommand(30,1,1) != HAL_OK) {
        DEBUG_PRINT("Failed to send charger command\n");
    }
    return pdFALSE;
}
static const CLI_Command_Definition_t sendChargerCLICommandDefinition =
{
    "chargeCommand",
    "chargeCommand:\r\n  send charge command to charger\r\n",
    sendChargerCLICommand,
    0 /* Number of parameters */
};

BaseType_t chargerCanStartCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    DEBUG_PRINT("Starting charger CAN\n");

    if (canStart(&CHARGER_CAN_HANDLE) != HAL_OK) {
        ERROR_PRINT("Failed to start charger can\n");
    }
    return pdFALSE;
}
static const CLI_Command_Definition_t chargerCanStartCommandDefinition =
{
    "canStartCharger",
    "canStartCharger:\r\n  Start charger can\r\n",
    chargerCanStartCommand,
    0 /* Number of parameters */
};


BaseType_t startCellBalancing(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEventISR(&fsmHandle, EV_Balance_Start);
    return pdFALSE;
}
static const CLI_Command_Definition_t startBalanceCommandDefinition =
{
    "startBalance",
    "startBalance:\r\n Start cell balancing\r\n",
    startCellBalancing,
    0 /* Number of parameters */
};


BaseType_t stopCellBalancing(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEventISR(&fsmHandle, EV_Balance_Stop);
    return pdFALSE;
}
static const CLI_Command_Definition_t stopBalanceCommandDefinition =
{
    "stopBalance",
    "stopBalance:\r\n Stop cell balancing command\r\n",
    stopCellBalancing,
    0 /* Number of parameters */
};

BaseType_t balanceCellCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    int cellIdx;

    const char *idxParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    sscanf(idxParam, "%u", &cellIdx);

    if (cellIdx < 0 || cellIdx >= NUM_VOLTAGE_CELLS) {
        COMMAND_OUTPUT("Cell Index must be between 0 and %d\n", NUM_VOLTAGE_CELLS-1);
        return pdFALSE;
    }

    const char * onOffParam = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);

    bool onOff = false;
    if (STR_EQ(onOffParam, "on", paramLen)) {
        onOff = true;
        COMMAND_OUTPUT("Setting cell: %d to balance\n",cellIdx);
    } else if (STR_EQ(onOffParam, "off", paramLen)) {
        onOff = false;
        COMMAND_OUTPUT("Setting cell: %d to not balance\n",cellIdx);
    } else {
        COMMAND_OUTPUT("Unkown parameter\n");
        return pdFALSE;
    }
    balance_cell(cellIdx, onOff);
    return pdFALSE;
}
static const CLI_Command_Definition_t balanceCellCommandDefinition =
{
    "balanceCell",
    "balanceCell <cell number> <on|off>:\r\n set the state of the balance ressistor for a specific cell\r\n",
    balanceCellCommand,
    2 /* Number of parameters */
};

BaseType_t hitlPrechargeModeCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;

    const char *packVoltageString = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    uint32_t intVPack;
    sscanf(packVoltageString, "%lu", &intVPack);

    HITL_VPACK = intVPack;
    HITL_Precharge_Mode = true;

    COMMAND_OUTPUT("HITL PC Mode, VPACK = %f\n", HITL_VPACK);
    return pdFALSE;
}
static const CLI_Command_Definition_t hitlPrechargeModeCommandDefinition =
{
    "hitlPC",
    "hitlPC <VPACK>:\r\n enables hitl precharge mode, setting pack voltage\r\n",
    hitlPrechargeModeCommand,
    1 /* Number of parameters */
};

BaseType_t bspdStatusCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    COMMAND_OUTPUT("BSPD State %s\n", getBSPD_Status()?"OK":"Fault");
    return pdFALSE;
}
static const CLI_Command_Definition_t bspdStatusCommandDefinition =
{
    "bspdStatus",
    "bspdStatus:\r\n get bspd status\r\n",
    bspdStatusCommand,
    0 /* Number of parameters */
};

BaseType_t tsmsStatusCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    COMMAND_OUTPUT("tsms State %s\n", getTSMS_Status()?"OK":"Fault");
    return pdFALSE;
}
static const CLI_Command_Definition_t tsmsStatusCommandDefinition =
{
    "tsmsStatus",
    "tsmsStatus:\r\n get tsms status\r\n",
    tsmsStatusCommand,
    0 /* Number of parameters */
};

BaseType_t hvdStatusCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    COMMAND_OUTPUT("hvd State %s\n", getHVD_Status()?"OK":"Fault");
    return pdFALSE;
}
static const CLI_Command_Definition_t hvdStatusCommandDefinition =
{
    "hvdStatus",
    "hvdStatus:\r\n get hvd status\r\n",
    hvdStatusCommand,
    0 /* Number of parameters */
};

BaseType_t sendCellCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;

    const char *cellIdxString = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    int cellIdx;
    sscanf(cellIdxString, "%d", &cellIdx);

    setSendOnlyOneCell(cellIdx);

    COMMAND_OUTPUT("Sending cell %d over CAN\n", cellIdx);
    return pdFALSE;
}
static const CLI_Command_Definition_t sendCellCommandDefinition =
{
    "sendCell",
    "sendCell <cellIdx>:\r\n Send over CAN one group of 3 cells (not all)\r\n",
    sendCellCommand,
    1 /* Number of parameters */
};

BaseType_t stopSendCellCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    clearSendOnlyOneCell();

    COMMAND_OUTPUT("Stopping sending only one cell group over CAN\n");
    return pdFALSE;
}
static const CLI_Command_Definition_t stopSendCellCommandDefinition =
{
    "stopSendCell",
    "stopSendCell <cellIdx>:\r\n Send over CAN all cells\r\n",
    stopSendCellCommand,
    0 /* Number of parameters */
};

extern bool skip_il;
BaseType_t forceChargeModeCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;

    const char *cellIdxString = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    int mode;
    sscanf(cellIdxString, "%d", &mode);

	setChargeMode(mode);
	skip_il = mode;
    return pdFALSE;
}

static const CLI_Command_Definition_t forceChargeModeCommandDefinition =
{
    "forceChargeMode",
    "forceChargeMode:\r\n Set precharge mode to 0: PC_MotorController, 1: PC_Charger. Force this change\r\n Please only do this if you know what you're doing\r\n",
    forceChargeModeCommand,
    1 /* Number of parameters */
};

extern volatile float limit_overvoltage;
BaseType_t setOverVoltageLimitCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;

    const char *cellIdxString = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    float limit;
    sscanf(cellIdxString, "%f", &limit);
	limit_overvoltage = limit;
    return pdFALSE;
}

static const CLI_Command_Definition_t setOverVoltageLimitCommandDefinition =
{
    "setOverVoltageLimit",
    "setOverVoltageLimit:\r\n Set the overvoltage limit for the battery (pass in a float) \r\n",
    setOverVoltageLimitCommand,
    1 /* Number of parameters */
};

extern volatile float limit_undervoltage;
BaseType_t setUnderVoltageLimitCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;

    const char *cellIdxString = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    float limit;
    sscanf(cellIdxString, "%f", &limit);
	limit_undervoltage = limit;
    return pdFALSE;
}

static const CLI_Command_Definition_t setUnderVoltageLimitCommandDefinition =
{
    "setUnderVoltageLimit",
    "setUnderVoltageLimit:\r\n Set the undervoltage limit for the battery (pass in a float) \r\n",
    setUnderVoltageLimitCommand,
    1 /* Number of parameters */
};


BaseType_t cbrbStatusCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    COMMAND_OUTPUT("cbrb State %s\n", getCBRB_Status()?"OK":"Fault");
    return pdFALSE;
}

static const CLI_Command_Definition_t cbrbStatusCommandDefinition =
{
    "cbrbStatus",
    "cbrbStatus:\r\n \r\n",
    cbrbStatusCommand,
    0 /* Number of parameters */
};

BaseType_t balanceCellsCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;


    const char * onOffParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    bool onOff = false;
    if (STR_EQ(onOffParam, "on", paramLen)) {
        onOff = true;
        COMMAND_OUTPUT("Setting all cells to balance\n");
    } else if (STR_EQ(onOffParam, "off", paramLen)) {
        onOff = false;
        COMMAND_OUTPUT("Setting all cells to not balance\n");
    } else {
        COMMAND_OUTPUT("Unkown parameter\n");
        return pdFALSE;
    }
    for(int i = 0;i < NUM_VOLTAGE_CELLS;i++)
	{
		balance_cell(i, onOff);
	}
    return pdFALSE;
}

static const CLI_Command_Definition_t balanceCellsCommandDefinition =
{
    "balanceCells",
    "balanceCells:\r\n \r\n",
    balanceCellsCommand,
    1 /* Number of parameters */
};


BaseType_t socCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
	COMMAND_OUTPUT("State of Charge %f %% \n", StateBatteryChargeHV);
    return pdFALSE;
}

static const CLI_Command_Definition_t socCommandDefinition =
{
    "soc",
    "soc:\r\n Print system state of charge \n",
    socCommand,
    0 /* Number of parameters */
};

BaseType_t getCellIRCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
	COMMAND_OUTPUT("AdjustedCellIR: %f (default %f)\n", adjustedCellIR, ADJUSTED_CELL_IR_DEFAULT);
    return pdFALSE;
}

static const CLI_Command_Definition_t getCellIRCommandDefinition =
{
    "getCellIR",
    "getCellIR:\r\n \r\n",
    getCellIRCommand,
    0 /* Number of parameters */
};

BaseType_t setCellIRCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char *newCellIR = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    float cellIR;
    sscanf(newCellIR, "%f", &cellIR);

    if (cellIR < 0.0 || cellIR > 0.01){
        COMMAND_OUTPUT("invalid cell IR [0,0.01]\r\n");
    }else{
	    adjustedCellIR = cellIR;
    }

    return pdFALSE;
}

static const CLI_Command_Definition_t setCellIRCommandDefinition =
{
    "setCellIR",
    "setCellIR: [0,0.01]\r\n \r\n",
    setCellIRCommand,
    1 /* Number of parameters */
};

BaseType_t getCellVoltages(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    // Make these static so their state persists across command calls
    static int cellIdx = -1;
    static float cell_voltages[NUM_VOLTAGE_CELLS];

    // First time the command is hit
    if (cellIdx == -1) {
        if (batt_spi_wakeup(true) != HAL_OK) {
            ERROR_PRINT("Failed to wake up boards\n");
            return pdFALSE;
        }

        // If the board was asleep, configuration is lost AND the reference is off.
        batt_write_config();

        // The first read broadcasts ADCV. Because the reference was off, the chip
        // takes t_REFUP (4.4ms) + t_CONV (2.5ms) = 6.9ms to finish. However, 
        // batt_read_cell_voltages only waits 2.5ms! This dummy read will likely 
        // return 0x8000 for the first registers, but importantly it forces the 
        // reference to power up.
        batt_read_cell_voltages(cell_voltages);
        
        // Wait an extra 5ms to ensure the delayed conversion from the first read 
        // finishes completely and doesn't interfere.
        vTaskDelay(pdMS_TO_TICKS(5));

        // Now the reference is fully powered up. This second read will complete 
        // within the normal 2.5ms and return valid measurements.
        if (batt_read_cell_voltages(cell_voltages) != HAL_OK) {
            COMMAND_OUTPUT("Error reading cell voltages\n");
            return pdFALSE;
        }
        
        COMMAND_OUTPUT("Cell Voltages:\n");
        cellIdx = 0;
        return pdTRUE; // Tell FreeRTOS CLI to call this function again
    }

    // Subsequent calls: output one cell at a time
    int board = cellIdx / CELLS_PER_BOARD;
    int cell = cellIdx % CELLS_PER_BOARD;
    COMMAND_OUTPUT("Board %d, Cell %d: %f V\n", board, cell, cell_voltages[cellIdx]);
    
    cellIdx++;

    // If we have printed all cells, return pdFALSE to stop
    if (cellIdx >= NUM_VOLTAGE_CELLS) {
        cellIdx = -1; // Reset for the next time the user runs the command
        return pdFALSE;
    }

    return pdTRUE; // More cells to print, call this function again
}

static const CLI_Command_Definition_t getCellVoltagesCommandDefinition =
{
    "getCellVoltages",
    "getCellVoltages:\r\n \r\n",
    getCellVoltages,
    0 /* Number of parameters */
};

BaseType_t getCellTemps(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    // Make these static so their state persists across command calls
    static float cell_temps[NUM_TEMP_CELLS];

    if (batt_spi_wakeup(true) != HAL_OK) {
        ERROR_PRINT("Failed to wake up boards\n");
        return pdFALSE;
    }

    if (batt_read_cell_temps(cell_temps) != HAL_OK) {
        COMMAND_OUTPUT("Error reading cell temperatures\n");
        return pdFALSE;
    }
    vTaskDelay(pdMS_TO_TICKS(50));

    DEBUG_PRINT("Cell Temperatures:\n");
    for(int i =0; i<1; i++){
        if (batt_read_cell_temps(cell_temps) != HAL_OK) {
            COMMAND_OUTPUT("Error reading cell temperatures\n");
            return pdFALSE;
        }
    }
    for(int i =0; i<NUM_TEMP_CELLS; i++){
        int board = i / THERMISTORS_PER_SEGMENT;
        int chip = i / SEGMENT_THERMISTORS_AMS1;
        int channel = i % SEGMENT_THERMISTORS_AMS1;
        DEBUG_PRINT("Board %d, Chip %d, Channel %d: %f degC\n", board, chip, channel, cell_temps[i]);
    }

    return pdFALSE;
}

static const CLI_Command_Definition_t getCellTempsCommandDefinition =
{
    "getCellTemps",
    "getCellTemps:\r\n Print all cell temperatures\r\n",
    getCellTemps,
    0 /* Number of parameters */
};

BaseType_t getCellVoltagesADSV(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    /* Same state machine as getCellVoltages: static 1D array, one line per CLI callback. */
    static int cellIdx = -1;
    static float cell_voltages[NUM_VOLTAGE_CELLS];

    if (cellIdx == -1) {
        if (batt_spi_wakeup(true) != HAL_OK) {
            ERROR_PRINT("Failed to wake up boards\n");
            return pdFALSE;
        }

        batt_write_config();

        /* Dummy ADSV capture (reference / pipeline warmup), same idea as dummy ADCV in getCellVoltages. */
        batt_read_cell_voltages_ADSV(cell_voltages);
        vTaskDelay(pdMS_TO_TICKS(5));

        if (batt_read_cell_voltages_ADSV(cell_voltages) != HAL_OK) {
            COMMAND_OUTPUT("Error reading cell voltages (ADSV)\n");
            return pdFALSE;
        }

        COMMAND_OUTPUT("Cell Voltages (ADSV):\n");
        cellIdx = 0;
        return pdTRUE;
    }

    int board = cellIdx / CELLS_PER_BOARD;
    int cell = cellIdx % CELLS_PER_BOARD;
    COMMAND_OUTPUT("Board %d, Cell %d: %f V\n", board, cell, cell_voltages[cellIdx]);

    cellIdx++;

    if (cellIdx >= NUM_VOLTAGE_CELLS) {
        cellIdx = -1;
        return pdFALSE;
    }

    return pdTRUE;
}
static const CLI_Command_Definition_t getCellVoltagesADSVCommandDefinition =
{
    "getCellVoltagesADSV",
    "getCellVoltagesADSV:\r\n Print all cell voltages\r\n",
    getCellVoltagesADSV,
    0 /* Number of parameters */
};

/**
 * @brief Manual PWM discharge for one global cell, same sequence as @ref handleCharge
 *        balance path (batteries.c): per-cell state, WRPWM, then discharge timer + WRCFGA/B.
 *        No RTOS delay/watchdog. Timer set to @ref DT_OFF so discharge runs until @ref stopDischargeCells.
 */
BaseType_t dischargeCellsCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    (void)writeBuffer;
    (void)writeBufferLength;

    BaseType_t paramLen;
    const char *cellIdxString = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    int req_cell;
    if (cellIdxString == NULL || sscanf(cellIdxString, "%d", &req_cell) != 1) {
        COMMAND_OUTPUT("dischargeCells <globalCell 0..%d>: manual PWM on one cell; stop with stopDischargeCells\r\n",
            NUM_VOLTAGE_CELLS - 1);
        return pdFALSE;
    }

    if (req_cell < 0 || req_cell >= NUM_VOLTAGE_CELLS) {
        COMMAND_OUTPUT("global cell must be 0..%d\r\n", NUM_VOLTAGE_CELLS - 1);
        return pdFALSE;
    }

#if IS_BOARD_F7
    /* Like battery task balance loop: for each cell either enable or stop discharge (one cell on, rest off). */

    if (batt_balance_cell(req_cell) != HAL_OK) {
        ERROR_PRINT("batt_discharge_cell %d failed\r\n", req_cell);
        return pdFALSE;
    }
    if (batt_spi_wakeup(true) != HAL_OK) {
        ERROR_PRINT("Failed to wake up boards\n");
        return pdFALSE;
    }
    if (batt_write_config_pwm() != HAL_OK) {
        ERROR_PRINT("batt_write_config_pwm: WRPWM A/B failed\n");
        return pdFALSE;
    }
    DEBUG_PRINT("Sent config to AMS boards (WRPWM)\r\n");

    /* Mirroring batteries.c: batt_set_disharge_timer + batt_write_config — use DT_OFF for no auto timeout. */
    if (batt_set_disharge_timer(DT_30_SEC) != HAL_OK) {
        ERROR_PRINT("batt_set_disharge_timer failed\n");
        return pdFALSE;
    }
    if (batt_write_config() != HAL_OK) {
        ERROR_PRINT("batt_write_config: WRCFGA/B failed\n");
        return pdFALSE;
    }
    DEBUG_PRINT("PWM discharge on global cell %d (DT_OFF, use getDischargeDcc / stopDischargeCells)\r\n", req_cell);
#else
    DEBUG_PRINT("dischargeCells: IS_BOARD_F7 only\r\n");
#endif
    return pdFALSE;
}

static const CLI_Command_Definition_t dischargeCellsCommandDefinition =
{
    "dischargeCells",
    "dischargeCells <globalCell>:\r\n One cell PWM discharge (WRPWM + WRCFG, DT_OFF); use stopDischargeCells to end\r\n",
    dischargeCellsCommand,
    1 /* Number of parameters */
};

BaseType_t stopDischargeCellsCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    (void)commandString;
    (void)writeBuffer;
    (void)writeBufferLength;

#if IS_BOARD_F7
    if (batt_unset_balancing_all_cells(15) != HAL_OK) {
        ERROR_PRINT("batt_unset_balancing_all_cells failed\n");
        return pdFALSE;
    }
    if (batt_spi_wakeup(true) != HAL_OK) {
        ERROR_PRINT("Failed to wake up boards\n");
        return pdFALSE;
    }
    if (batt_write_config_pwm() != HAL_OK) {
        ERROR_PRINT("batt_write_config_pwm: WRPWM A/B failed\n");
        return pdFALSE;
    }
    if (batt_set_disharge_timer(DT_OFF) != HAL_OK) {
        ERROR_PRINT("batt_set_disharge_timer failed\n");
        return pdFALSE;
    }
    if (batt_write_config() != HAL_OK) {
        ERROR_PRINT("batt_write_config: WRCFGA/B failed\n");
        return pdFALSE;
    }
    COMMAND_OUTPUT("Stopped PWM discharge on all cells; WRCFG sent\r\n");
#else
    COMMAND_OUTPUT("stopDischargeCells: IS_BOARD_F7 only\r\n");
#endif
    return pdFALSE;
}

static const CLI_Command_Definition_t stopDischargeCellsCommandDefinition =
{
    "stopDischargeCells",
    "stopDischargeCells:\r\n Clear all PWM discharge bits (WRPWM + WRCFG, DT_OFF)\r\n",
    stopDischargeCellsCommand,
    0 /* Number of parameters */
};

BaseType_t getDischargeDccCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    static int get_dcc_cli_idx = -1;
    static uint8_t get_pwm_duty[NUM_VOLTAGE_CELLS];

    (void)commandString;
    (void)writeBufferLength;

    if (get_dcc_cli_idx == -1) {
#if LTC_CHIP == ADBMS_CHIP_6830B
        uint8_t all_pwma[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE];
        uint8_t all_pwmb[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE];

        if (batt_spi_wakeup(true) != HAL_OK) {
            ERROR_PRINT("Failed to wake up boards\n");
            return pdFALSE;
        }
        if (batt_read_pwm(all_pwma, all_pwmb) != HAL_OK) {
            COMMAND_OUTPUT("Error reading AMS RDPWM\r\n");
            return pdFALSE;
        }
        for (int g = 0; g < NUM_VOLTAGE_CELLS; g++) {
            get_pwm_duty[g] = (uint8_t)batt_pwm_duty_from_pwm_readback(g, all_pwma, all_pwmb);
        }
        COMMAND_OUTPUT("PWM duty 0..15 per cell (RDPWMA/RDPWMB readback):\r\n");
#else
        COMMAND_OUTPUT("getDischargeDcc uses RDPWM readback (ADBMS6830 only)\r\n");
        return pdFALSE;
#endif
        get_dcc_cli_idx = 0;
        return pdTRUE;
    }

    int board = get_dcc_cli_idx / CELLS_PER_BOARD;
    int cell = get_dcc_cli_idx % CELLS_PER_BOARD;
    COMMAND_OUTPUT("Board %d, Cell %d: PWM %u\r\n", board, cell, (unsigned)get_pwm_duty[get_dcc_cli_idx]);
    get_dcc_cli_idx++;
    if (get_dcc_cli_idx >= NUM_VOLTAGE_CELLS) {
        get_dcc_cli_idx = -1;
        return pdFALSE;
    }
    return pdTRUE;
}

static const CLI_Command_Definition_t getDischargeDccCommandDefinition =
{
    "getDischargeDcc",
    "getDischargeDcc:\r\n Per-cell PWM duty (0-15) from RDPWMA/B (6830); paged output\r\n",
    getDischargeDccCommand,
    0 /* Number of parameters */
};

BaseType_t getThermalShutdownCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    static int thermal_cli_idx = -1;
    static uint8_t thermal_sd[NUM_DEVICES];

    (void)commandString;
    (void)writeBufferLength;

    if (thermal_cli_idx == -1) {
        uint8_t statc[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][STATUS_SIZE];

        if (batt_spi_wakeup(true) != HAL_OK) {
            ERROR_PRINT("Failed to wake up boards\n");
            return pdFALSE;
        }
        if (batt_read_rdstatc(statc) != HAL_OK) {
            COMMAND_OUTPUT("Error reading RDSTATC\r\n");
            return pdFALSE;
        }
        for (int b = 0; b < NUM_BOARDS; b++) {
            for (int c = 0; c < NUM_LTC_CHIPS_PER_BOARD; c++) {
                int i = b * NUM_LTC_CHIPS_PER_BOARD + c;
                thermal_sd[i] = (uint8_t)rdstatc_thermal_shutdown(statc[b][c]);
            }
        }
        COMMAND_OUTPUT("RDSTATC thermal SD (byte5 bit2, 1=active):\n");
        thermal_cli_idx = 0;
        return pdTRUE;
    }

    int board = thermal_cli_idx / NUM_LTC_CHIPS_PER_BOARD;
    int chip = thermal_cli_idx % NUM_LTC_CHIPS_PER_BOARD;
    COMMAND_OUTPUT("Board %d Chip %d: %u\n", board, chip, (unsigned)thermal_sd[thermal_cli_idx]);
    thermal_cli_idx++;
    if (thermal_cli_idx >= NUM_DEVICES) {
        thermal_cli_idx = -1;
        return pdFALSE;
    }
    return pdTRUE;
}

static const CLI_Command_Definition_t getThermalShutdownCommandDefinition =
{
    "getThermalShutdown",
    "getThermalShutdown:\r\n RDSTATC thermal shutdown per device (paged)\r\n",
    getThermalShutdownCommand,
    0 /* Number of parameters */
};

BaseType_t readAmsConfigCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    static uint8_t configA[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};
    static uint8_t configB[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};

    if (batt_spi_wakeup(true) != HAL_OK) {
        ERROR_PRINT("Failed to wake up boards\n");
        return pdFALSE;
    }
    
    // Read config from AMS boards
    HAL_StatusTypeDef status = batt_read_config(configA, configB);
    
    if (status != HAL_OK) {
        ERROR_PRINT("Warning: Error reading AMS config tables. Printing whatever data was retrieved.\n");
    }

    DEBUG_PRINT("AMS Configuration Tables:\n");
    DEBUG_PRINT("========================\n\n");

    for (int board = 0; board < NUM_BOARDS; board++) {
        for(int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++) {
            DEBUG_PRINT("Board %d:\n", board);
            DEBUG_PRINT("Chip %d:\n", chip);
            DEBUG_PRINT("  Config A: ");
            for (int i = 0; i < BATT_CONFIG_SIZE; i++) {
                DEBUG_PRINT("%02X ", configA[board][chip][i]);
            }
            DEBUG_PRINT("\n");
            DEBUG_PRINT("  Config B: ");
            for (int i = 0; i < BATT_CONFIG_SIZE; i++) {
                DEBUG_PRINT("%02X ", configB[board][chip][i]);
            }
            DEBUG_PRINT("\n\n");
        }
    }

    return pdFALSE;
}

static const CLI_Command_Definition_t readAmsConfigCommandDefinition =
{
    "readAmsConfig",
    "readAmsConfig:\r\n Read and display AMS config tables A and B\r\n",
    readAmsConfigCommand,
    0 /* Number of parameters */
};

BaseType_t verifyAmsConfigCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    static uint8_t configA[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};
    static uint8_t configB[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};

    if (batt_spi_wakeup(true) != HAL_OK) {
        ERROR_PRINT("Failed to wake up boards\n");
        return pdFALSE;
    }
    batt_init_chip_configs();
    batt_init_chip_configs_pwm();
    
    if (batt_write_config() != HAL_OK) {
        ERROR_PRINT("Warning: Error writing AMS config tables.\n");
    }

    long_delay_us(2480);

    if (batt_spi_wakeup(true) != HAL_OK) {
        ERROR_PRINT("Failed to wake up boards\n");
        return pdFALSE;
    }
    
    // Read config from AMS boards
    HAL_StatusTypeDef status = batt_read_config(configA, configB);
    
    if (status != HAL_OK) {
        DEBUG_PRINT("Warning: Error reading AMS config tables. Printing whatever data was retrieved.\n");
    }

    DEBUG_PRINT("AMS (verify) Configuration Tables:\n");
    DEBUG_PRINT("========================\n\n");

    for (int board = 0; board < NUM_BOARDS; board++) {
        for(int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++) {

            DEBUG_PRINT("Board %d:\n", board);
            DEBUG_PRINT("Chip %d:\n", chip);
            DEBUG_PRINT("  Config A: ");
            for (int i = 0; i < BATT_CONFIG_SIZE; i++) {
                DEBUG_PRINT("%02X ", configA[board][chip][i]);
            }
            DEBUG_PRINT("\n");
            DEBUG_PRINT("  Config B: ");
            for (int i = 0; i < BATT_CONFIG_SIZE; i++) {
                DEBUG_PRINT("%02X ", configB[board][chip][i]);
            }
            DEBUG_PRINT("\n\n");
        }
    }

    return pdFALSE;
}

static const CLI_Command_Definition_t verifyAmsConfigCommandDefinition =
{
    "verifyAmsConfig",
    "verifyAmsConfig:\r\n Verify AMS config tables A and B\r\n",
    verifyAmsConfigCommand,
    0 /* Number of parameters */
};

BaseType_t calcDataPecCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    uint8_t data[6];
    uint8_t pec[2];
    
    // Parse the 6 parameters
    for (int i = 0; i < 6; i++) {
        const char * param = FreeRTOS_CLIGetParameter(commandString, i + 1, &paramLen);
        if (param == NULL) {
            COMMAND_OUTPUT("Error: Must provide exactly 6 hex bytes\n");
            return pdFALSE;
        }
        unsigned int val;
        // Parse hex directly (e.g. 01 00 00 FF 03 00 or 0x01 ... )
        if (sscanf(param, "%x", &val) != 1) {
            COMMAND_OUTPUT("Error: Failed to parse byte %d\n", i);
            return pdFALSE;
        }
        data[i] = (uint8_t)val;
    }

    // Run the Data PEC generator
    batt_gen_pec_data(data, 6, pec, 0);

    COMMAND_OUTPUT("Data: %02X %02X %02X %02X %02X %02X\n", 
            data[0], data[1], data[2], data[3], data[4], data[5]);
    COMMAND_OUTPUT("Calculated Data PEC (10-bit): %02X %02X\n", pec[0], pec[1]);

    return pdFALSE;
}

static const CLI_Command_Definition_t calcDataPecCommandDefinition =
{
    "calcDataPec",
    "calcDataPec <b0> <b1> <b2> <b3> <b4> <b5>:\r\n Calculates 10-bit Data PEC for 6 hex bytes\r\n",
    calcDataPecCommand,
    6 /* Number of parameters */
};


HAL_StatusTypeDef stateMachineMockInit()
{
    cliSetVBatt(0);
    cliSetVBus(0);
    cliSetIBus(0);

    if (FreeRTOS_CLIRegisterCommand(&debugUartOverCanCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
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
    if (FreeRTOS_CLIRegisterCommand(&mockFsmEventCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&printStateCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&setChannelTempCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&setCellVoltageCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&printBattInfoCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&testLowPassFilterCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&brakePressureCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }    
    if (FreeRTOS_CLIRegisterCommand(&setPosContCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&setNegContCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&setPCDCCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getBrakePressureCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
#if IS_BOARD_F7
    if (FreeRTOS_CLIRegisterCommand(&setImdErrorThresholdCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getImdErrorThresholdCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
#endif
    if (FreeRTOS_CLIRegisterCommand(&stopChargeCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&startChargeCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&maxChargeCurrentCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&chargeCartHeartbeatMockCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&sendChargerCLICommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&chargerCanStartCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&startBalanceCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&stopBalanceCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&balanceCellCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&hitlPrechargeModeCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&bspdStatusCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&tsmsStatusCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&hvdStatusCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&sendCellCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&stopSendCellCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&fakeEnterChargeModeCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&forceChargeModeCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&setOverVoltageLimitCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&setUnderVoltageLimitCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&cbrbStatusCommandDefinition) != pdPASS) {
		return HAL_ERROR;
	}
    if (FreeRTOS_CLIRegisterCommand(&socCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&balanceCellsCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&setStateBusHVSendPeriodCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getStateBusHVSendPeriodCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getCellIRCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&setCellIRCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getCellVoltagesCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getCellTempsCommandDefinition) != pdPASS) {
        DEBUG_PRINT("getCellTempsCommandDefinition failed\n");
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getCellVoltagesADSVCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&dischargeCellsCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&stopDischargeCellsCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getDischargeDccCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getThermalShutdownCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&readAmsConfigCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&verifyAmsConfigCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&calcDataPecCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    return HAL_OK;
}
