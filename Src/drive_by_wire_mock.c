#include "drive_by_wire_mock.h"
#include "drive_by_wire.h"
#include "debug.h"
#include "string.h"
#include "state_machine.h"
#include "FreeRTOS_CLI.h"
#include "task.h"
#include "cmsis_os.h"
#include "VCU_F7_can.h"
#include "brakeAndThrottle.h"

extern osThreadId driveByWireHandle;
extern uint32_t brakeThrottleSteeringADCVals[NUM_ADC_CHANNELS];

BaseType_t setBrakePosition(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    float brakePosition;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(param, "%f", &brakePosition);
    brakeThrottleSteeringADCVals[BRAKE_POS_INDEX] = brakePosition * TPS_DIVISOR / TPS_MULTPLIER;
    COMMAND_OUTPUT("Setting brake to %f %%, (adcVal: %lu)\n", brakePosition, brakeThrottleSteeringADCVals[BRAKE_POS_INDEX]);

    return pdFALSE;
}
static const CLI_Command_Definition_t brakePositionCommandDefinition =
{
    "brake",
    "brake <val>:\r\n Set brake Position to val %\r\n",
    setBrakePosition,
    1 /* Number of parameters */
};

BaseType_t pduMCsOnOffMock(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    if (STR_EQ(param, "on", paramLen)) {
        xTaskNotify( driveByWireHandle,
                            (1<<NTFY_MCs_ON),
                            eSetBits );
    } else if (STR_EQ(param, "off", paramLen)) {
        xTaskNotify( driveByWireHandle,
                            (1<<NTFY_MCs_OFF),
                            eSetBits );
    } else {
        COMMAND_OUTPUT("Unknown parameter\n");
        return pdFALSE;
    }

    return pdFALSE;
}
static const CLI_Command_Definition_t mcOnOffCommandDefinition =
{
    "mc",
    "mc <on|off>:\r\n Send notify that MCs are on|off (normal sent by PDU)\r\n",
    pduMCsOnOffMock,
    1 /* Number of parameters */
};

BaseType_t setFakeThrottle(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    float throttlePercent;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(param, "%f", &throttlePercent);
    brakeThrottleSteeringADCVals[THROTTLE_A_INDEX] = calculate_throttle_adc_from_percent1(throttlePercent);
    brakeThrottleSteeringADCVals[THROTTLE_B_INDEX] = calculate_throttle_adc_from_percent2(throttlePercent);
    COMMAND_OUTPUT("Setting throttle %f (ADC A: %lu, ADC B: %lu)\n", throttlePercent, brakeThrottleSteeringADCVals[THROTTLE_A_INDEX], brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]);

    return pdFALSE;
}
static const CLI_Command_Definition_t throttleCommandDefinition =
{
    "throttle",
    "throttle <val>:\r\n Set throttle to val (sets both throttle pots to same val)\r\n",
    setFakeThrottle,
    1 /* Number of parameters */
};

BaseType_t setFakeThrottleAB(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    float throttlePercent1, throttlePercent2;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(param, "%f", &throttlePercent1);
    brakeThrottleSteeringADCVals[THROTTLE_A_INDEX] = calculate_throttle_adc_from_percent1(throttlePercent1);

    param = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);

    sscanf(param, "%f", &throttlePercent2);
    brakeThrottleSteeringADCVals[THROTTLE_B_INDEX] = calculate_throttle_adc_from_percent2(throttlePercent2);

    COMMAND_OUTPUT("Setting throttle A %f (ADC: %lu), B %f (ADC: %lu)\n", throttlePercent1, brakeThrottleSteeringADCVals[THROTTLE_A_INDEX], throttlePercent2, brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]);

    return pdFALSE;
}
static const CLI_Command_Definition_t throttleABCommandDefinition =
{
    "throttleAB",
    "throttleAB <A Val> <B Val>:\r\n Set throttle pots A and B seperately\r\n",
    setFakeThrottleAB,
    2 /* Number of parameters */
};

BaseType_t getFakeThrottleAB(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    COMMAND_OUTPUT("Throttle A %u (ADC: %lu), B %u (ADC: %lu)\n",
                   calculate_throttle_percent1(brakeThrottleSteeringADCVals[THROTTLE_A_INDEX]),
                   brakeThrottleSteeringADCVals[THROTTLE_A_INDEX],
                   calculate_throttle_percent2(brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]),
                   brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]);

    /*COMMAND_OUTPUT("Vals: %lu, %lu, %lu, %lu, %lu\n", brakeThrottleSteeringADCVals[0],*/
                   /*brakeThrottleSteeringADCVals[1], brakeThrottleSteeringADCVals[2],*/
                   /*brakeThrottleSteeringADCVals[3], brakeThrottleSteeringADCVals[4]);*/
    return pdFALSE;
}
static const CLI_Command_Definition_t getThrottleABCommandDefinition =
{
    "getThrottleAB",
    "getThrottleAB:\r\n Get throttle pots A and B\r\n",
    getFakeThrottleAB,
    0 /* Number of parameters */
};

BaseType_t setFakeBrakePressure(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    uint32_t pressure;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(param, "%lu", &pressure);

    COMMAND_OUTPUT("setting brake pressure %lu\n", pressure);
    if (pressure < MIN_BRAKE_PRESSURE) {
        fsmSendEventISR(&fsmHandle, EV_Brake_Pressure_Fault);
    }
    brakeThrottleSteeringADCVals[BRAKE_PRES_INDEX] = pressure * BRAKE_PRESSURE_DIVIDER / BRAKE_PRESSURE_MULTIPLIER;

    return pdFALSE;
}
static const CLI_Command_Definition_t brakePressureCommandDefinition =
{
    "brakePressure",
    "brakePressure <val>:\r\n Set brake pressure to val\r\n",
    setFakeBrakePressure,
    1 /* Number of parameters */
};

BaseType_t setFakeDCUCanTimeout(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEventISR(&fsmHandle, EV_DCU_Can_Timeout);
    return pdFALSE;
}
static const CLI_Command_Definition_t dcuTimeoutCommandDefinition =
{
    "dcuTimeout",
    "dcuTimeout:\r\n Send dcu timeout event\r\n",
    setFakeDCUCanTimeout,
    0 /* Number of parameters */
};

BaseType_t fakeEM_ToggleDCU(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEventISR(&fsmHandle, EV_EM_Toggle);
    return pdFALSE;
}
static const CLI_Command_Definition_t emToggleCommandDefinition =
{
    "emToggle",
    "emToggle:\r\n Send em toggle event\r\n",
    fakeEM_ToggleDCU,
    0 /* Number of parameters */
};

BaseType_t fakeHVStateChange(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    bool newHVState;
    BaseType_t paramLen;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    if (STR_EQ(param, "enable", paramLen)) {
        newHVState = true;
    } else if (STR_EQ(param, "disable", paramLen)) {
        newHVState = false;
        fsmSendEventISR(&fsmHandle, EV_Hv_Disable);
    } else {
        COMMAND_OUTPUT("Unknown parameter\n");
        return pdFALSE;
    }
    if (HV_Power_State == HV_Power_State_On && !newHVState) {
        fsmSendEventISR(&fsmHandle, EV_Hv_Disable);
    }

    if (newHVState) {
        HV_Power_State = HV_Power_State_On;
    } else{
        HV_Power_State = HV_Power_State_Off;
    }

    return pdFALSE;
}
static const CLI_Command_Definition_t hvStateCommandDefinition =
{
    "hv",
    "hv <enable|disable>:\r\n Send hv state change event, and set hv state\r\n",
    fakeHVStateChange,
    1 /* Number of parameters */
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
    if (FreeRTOS_CLIRegisterCommand(&throttleABCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&brakePressureCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&throttleCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&dcuTimeoutCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&emToggleCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&hvStateCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&printStateCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&mcOnOffCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&brakePositionCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getThrottleABCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    return HAL_OK;
}
