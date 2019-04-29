#include "controlStateMachine_mock.h"
#include "debug.h"
#include "string.h"
#include "state_machine.h"
#include "FreeRTOS_CLI.h"
#include "task.h"
#include "cmsis_os.h"
#include "prechargeDischarge.h"
#include "BMU_can.h"
#include "controlStateMachine.h"
#include "testData.h"
#include "filters.h"
#include "sense.h"

extern float IBus;
extern float VBus;
extern float VBatt;
extern uint32_t brakeAndHVILVals[2];

BaseType_t getBrakePressure(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    COMMAND_OUTPUT("Brake %f %%, (adcVal: %lu)\n",
                   ((float)brakeAndHVILVals[BRAKE_ADC_CHANNEL]) / BRAKE_ADC_DIVIDER,
                   brakeAndHVILVals[BRAKE_ADC_CHANNEL]);

    return pdFALSE;
}
static const CLI_Command_Definition_t getBrakePressureCommandDefinition =
{
    "getBrake",
    "getBrake:\r\n Get brake pressure\r\n",
    getBrakePressure,
    0 /* Number of parameters */
};

BaseType_t setBrakePressure(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    float brakePressure;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(param, "%f", &brakePressure);
    brakeAndHVILVals[BRAKE_ADC_CHANNEL] = brakePressure * BRAKE_ADC_DIVIDER;
    COMMAND_OUTPUT("Setting brake to %f %%, (adcVal: %lu)\n", brakePressure, brakeAndHVILVals[BRAKE_ADC_CHANNEL]);

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
    _Static_assert(VOLTAGECELL_COUNT == TEMPCELL_COUNT, "Length of array for cell voltages doens't match array for cell temps");

    static int cellIdx = -5;

    if (cellIdx == -5) {
        COMMAND_OUTPUT("IBUS\tVBUS\tVBATT\r\n");
        cellIdx = -4;
        return pdTRUE;
    } else if (cellIdx == -4) {
        COMMAND_OUTPUT("%f\t%f\t%f\r\n\n", IBus, VBus, VBatt);
        cellIdx = -3;
        return pdTRUE;
    } else if (cellIdx == -3) {
        COMMAND_OUTPUT("MinVoltage\tMaxVoltage\tMinTemp\tMaxTemp\r\n");
        cellIdx = -2;
        return pdTRUE;
    } else if (cellIdx == -2) {
        COMMAND_OUTPUT("%f\t%f\t%f\t%f\r\n\n", VoltageCellMin, VoltageCellMax, TempCellMin, TempCellMax);
        cellIdx = -1;
        return pdTRUE;
    } else if (cellIdx == -1) {
        COMMAND_OUTPUT("Cell\tVoltage(V)\tTemp(degC)\r\n");
        cellIdx = 0;
        return pdTRUE;
    }

    COMMAND_OUTPUT("%d\t%f\t%f\r\n", cellIdx, VoltageCell[cellIdx], TempCell[cellIdx]);

    if (++cellIdx >= VOLTAGECELL_COUNT) {
        cellIdx = -3;
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

    if (cellIdx < 0 || cellIdx >= VOLTAGECELL_COUNT) {
        COMMAND_OUTPUT("Cell Index must be between 0 and %d\n", VOLTAGECELL_COUNT);
        return pdFALSE;
    }

    sscanf(voltageParam, "%f", &VoltageCell[cellIdx]);
    COMMAND_OUTPUT("VoltageCell[%d] = %fV\n", cellIdx, VoltageCell[cellIdx]);
    return pdFALSE;
}
static const CLI_Command_Definition_t setCellVoltageCommandDefinition =
{
    "voltageCell",
    "voltageCell <idx> <voltage>:\r\n Set a cells voltage\r\n",
    setCellVoltage,
    2 /* Number of parameters */
};

BaseType_t setCellTemp(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    int cellIdx;

    const char *idxParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    const char *tempParam = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);

    sscanf(idxParam, "%u", &cellIdx);

    if (cellIdx < 0 || cellIdx >= TEMPCELL_COUNT) {
        COMMAND_OUTPUT("Cell Index must be between 0 and %d\n", TEMPCELL_COUNT);
        return pdFALSE;
    }

    sscanf(tempParam, "%f", &TempCell[cellIdx]);
    COMMAND_OUTPUT("TempCell[%d] = %fdegC\n", cellIdx, TempCell[cellIdx]);
    return pdFALSE;
}
static const CLI_Command_Definition_t setCellTempCommandDefinition =
{
    "tempCell",
    "tempCell <idx> <temp>:\r\n Set a cells temperature\r\n",
    setCellTemp,
    2 /* Number of parameters */
};

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
    IBus = current;

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
            COMMAND_OUTPUT("Cell Index must be between 0 and %d\n", VOLTAGECELL_COUNT);
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
            COMMAND_OUTPUT("Cell Index must be between 0 and %d\n", VOLTAGECELL_COUNT);
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
            COMMAND_OUTPUT("Cell Index must be between 0 and %d\n", VOLTAGECELL_COUNT);
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



HAL_StatusTypeDef stateMachineMockInit()
{
    IBus = 0;
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
    if (FreeRTOS_CLIRegisterCommand(&setCellTempCommandDefinition) != pdPASS) {
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

    return HAL_OK;
}
