#include "drive_by_wire_mock.h"
#include "drive_by_wire.h"
#include "debug.h"
#include "string.h"
#include "state_machine.h"
#include "FreeRTOS_CLI.h"
#include "task.h"
#include "cmsis_os.h"
#include "VCU_F7_can.h"

extern osThreadId driveByWireHandle;

volatile bool fakeBPSState = true;

volatile int fakeBrakePressure = 100;
volatile int fakeThrottle = 0;

volatile HAL_StatusTypeDef fakeThrottleSuccess = HAL_OK;

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
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(param, "%u", &fakeThrottle);
    COMMAND_OUTPUT("Setting throttle %u\n", fakeThrottle);

    return pdFALSE;
}
static const CLI_Command_Definition_t throttleCommandDefinition =
{
    "throttle",
    "throttle <val>:\r\n Set throttle to val\r\n",
    setFakeThrottle,
    1 /* Number of parameters */
};

BaseType_t setFakeThrottleSuccess(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    bool newState;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    if (STR_EQ(param, "ok", paramLen)) {
        newState = true;
    } else if (STR_EQ(param, "fail", paramLen)) {
        newState = false;
    } else {
        COMMAND_OUTPUT("Unknown parameter\n");
        return pdFALSE;
    }

    DEBUG_PRINT("setting throttle success state %d\n", newState);
    fakeThrottleSuccess = newState;
    if (!newState) {
        fsmSendEvent(&fsmHandle, EV_Throttle_Failure, portMAX_DELAY);
    }

    return pdFALSE;
}
static const CLI_Command_Definition_t throttleStateCommandDefinition =
{
    "throttleState",
    "throttleState <ok|fail>:\r\n Set throttle state\r\n",
    setFakeThrottleSuccess,
    1 /* Number of parameters */
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
    fakeBrakePressure = pressure;

    return pdFALSE;
}
static const CLI_Command_Definition_t brakePressureCommandDefinition =
{
    "brakePressure",
    "brakePressure <val>:\r\n Set brake pressure to val\r\n",
    setFakeBrakePressure,
    1 /* Number of parameters */
};

BaseType_t setFakeBPSState(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    bool newBpsState;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    if (STR_EQ(param, "ok", paramLen)) {
        newBpsState = true;
    } else if (STR_EQ(param, "fail", paramLen)) {
        newBpsState = false;
    } else {
        COMMAND_OUTPUT("Unknown parameter\n");
        return pdFALSE;
    }

    if (fakeBPSState && newBpsState != fakeBPSState) {
        fsmSendEventISR(&fsmHandle, EV_Bps_Fail);
    }

    fakeBPSState = newBpsState;

    return pdFALSE;
}
static const CLI_Command_Definition_t bpsCommandDefinition =
{
    "bps",
    "bps <fail|ok>:\r\n Set bps state\r\n",
    setFakeBPSState,
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

// Mock functions
int getThrottle() {
    return fakeThrottle;
}
int getBrakePressure() {
    return fakeBrakePressure;
}
HAL_StatusTypeDef outputThrottle() {
    return fakeThrottleSuccess;
}
bool checkBPSState() {
    return fakeBPSState;
}
bool throttle_is_zero() {
    return (fakeThrottle==0);
}

HAL_StatusTypeDef stateMachineMockInit()
{
    if (FreeRTOS_CLIRegisterCommand(&bpsCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&throttleStateCommandDefinition) != pdPASS) {
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

    return HAL_OK;
}
