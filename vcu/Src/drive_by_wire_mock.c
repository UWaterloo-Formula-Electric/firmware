#include "drive_by_wire_mock.h"
#include "drive_by_wire.h"
#include "debug.h"
#include "string.h"
#include "state_machine.h"
#include "FreeRTOS_CLI.h"
#include "task.h"
#include "cmsis_os.h"
#include "vcu_F7_can.h"
#include "brakeAndThrottle.h"
#include "bsp.h"
#include "motorController.h"
#include "traction_control.h"
#include "motorController.h"
#include <math.h>

extern osThreadId driveByWireHandle;
extern uint32_t brakeThrottleSteeringADCVals[NUM_ADC_CHANNELS];

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

BaseType_t getThrottle(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    float throttle = 0;
    ThrottleStatus_t rc = getNewThrottle(&throttle);
    COMMAND_OUTPUT("Throttle %f, status (%s)\n", throttle, rc==THROTTLE_OK?"OK":"FAIL");

    /*COMMAND_OUTPUT("Vals: %lu, %lu, %lu, %lu, %lu\n", brakeThrottleSteeringADCVals[0],*/
                   /*brakeThrottleSteeringADCVals[1], brakeThrottleSteeringADCVals[2],*/
                   /*brakeThrottleSteeringADCVals[3], brakeThrottleSteeringADCVals[4]);*/
    return pdFALSE;
}
static const CLI_Command_Definition_t getThrottleCommandDefinition =
{
    "getThrottle",
    "getThrottle:\r\n Get throttle and throttle state\r\n",
    getThrottle,
    0 /* Number of parameters */
};

BaseType_t getSteering(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    float steeringAngle = getSteeringAngle();
    COMMAND_OUTPUT("Steering Angle: %f degrees\n", steeringAngle);

    return pdFALSE;
}
static const CLI_Command_Definition_t getSteeringCommandDefinition =
{
    "getSteering",
    "getSteering:\r\n Get steering angle in degrees\r\n",
    getSteering,
    0 /* Number of parameters */
};

BaseType_t getBrake(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    float brake = getBrakePositionPercent();
    float brakePressure = getBrakePressurePercent();
    float brakePosition = getBrakePotentiometerPercent();
    COMMAND_OUTPUT("Brake %f (pressure: %f, position: %f)\n",
                   brake,
                   brakePressure,
                   brakePosition);

    /*COMMAND_OUTPUT("Vals: %lu, %lu, %lu, %lu, %lu\n", brakeThrottleSteeringADCVals[0],*/
                   /*brakeThrottleSteeringADCVals[1], brakeThrottleSteeringADCVals[2],*/
                   /*brakeThrottleSteeringADCVals[3], brakeThrottleSteeringADCVals[4]);*/
    return pdFALSE;
}
static const CLI_Command_Definition_t getBrakeCommandDefinition =
{
    "getBrake",
    "getBrake:\r\n Get brake position\r\n",
    getBrake,
    0 /* Number of parameters */
};

BaseType_t getFakeThrottleAB(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    uint32_t thA = ADC_12_BIT_2_12_BIT(brakeThrottleSteeringADCVals[THROTTLE_A_INDEX]);
    uint32_t thB = ADC_12_BIT_2_12_BIT(brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]);

    COMMAND_OUTPUT("Throttle A %f (ADC: %lu), B %f (ADC: %lu)\n",
                   calculate_throttle_percent1(thA),
                   thA,
                   calculate_throttle_percent2(thB),
                   thB);

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

BaseType_t getADCInputs(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{

    uint32_t thA = getThrottleAFiltered();
    uint32_t thB = getThrottleBFiltered();

    DEBUG_PRINT("Throttle A %f (ADC: %lu), B %f (ADC: %lu)\n",
                calculate_throttle_percent1(thA),
                thA,
                calculate_throttle_percent2(thB),
                thB);
    DEBUG_PRINT("Brake Pos: %lu, brake pres: %lu, steering pos: %lu\n",
                brakeThrottleSteeringADCVals[BRAKE_POS_INDEX],
                brakeThrottleSteeringADCVals[BRAKE_PRES_INDEX],
                brakeThrottleSteeringADCVals[STEERING_INDEX]);

    /*COMMAND_OUTPUT("Vals: %lu, %lu, %lu, %lu, %lu\n", brakeThrottleSteeringADCVals[0],*/
                   /*brakeThrottleSteeringADCVals[1], brakeThrottleSteeringADCVals[2],*/
                   /*brakeThrottleSteeringADCVals[3], brakeThrottleSteeringADCVals[4]);*/
    return pdFALSE;
}
static const CLI_Command_Definition_t getADCInputsCommandDefinition =
{
    "adcInputs",
    "adcInputs:\r\n Get adc input values\r\n",
    getADCInputs,
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
    if (pressure < MIN_BRAKE_PRESSURE_PSI) {
        fsmSendEventISR(&VCUFsmHandle, EV_Brake_Pressure_Fault);
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

BaseType_t fakeHvToggle(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
	fsmSendEvent(&VCUFsmHandle, EV_BTN_HV_Toggle, portMAX_DELAY);
	return pdFALSE;
}
static const CLI_Command_Definition_t fakeHvToggleCommandDefinition =
{
	"hv",
	"hv \r\n fake press of HV Toggle\r\n",
	fakeHvToggle,
	0 /*Number of parameters*/
};

BaseType_t fakeEM_Toggle(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    fsmSendEventISR(&VCUFsmHandle, EV_EM_Toggle);
    return pdFALSE;
}
static const CLI_Command_Definition_t emToggleCommandDefinition =
{
    "emToggle",
    "emToggle:\r\n Send em toggle event\r\n",
    fakeEM_Toggle,
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
        fsmSendEventISR(&VCUFsmHandle, EV_Hv_Disable);
    } else {
        COMMAND_OUTPUT("Unknown parameter\n");
        return pdFALSE;
    }
    if (HV_Power_State == HV_Power_State_On && !newHVState) {
        fsmSendEventISR(&VCUFsmHandle, EV_Hv_Disable);
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
    uint8_t index = fsmGetState(&VCUFsmHandle);
    if (index >= 0 && index < STATE_ANY){
        COMMAND_OUTPUT("State: %s\n", VCU_States_String[index]);
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
    0/* Number of parameters */
};

BaseType_t torqueDemandMaxCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * torqueMaxString = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    uint64_t maxTorqueDemand;
    sscanf(torqueMaxString, "%llu", &maxTorqueDemand);

    if(maxTorqueDemand > MAX_MOTOR_TORQUE_NM){
        COMMAND_OUTPUT("Max torque input out of range, must be between 0 and 231Nm");
    }else{
        setTorqueLimit(maxTorqueDemand);    
        COMMAND_OUTPUT("Setting max torque demand to %llu (Nm)\n", maxTorqueDemand); 
    }
    return pdFALSE;
}
static const CLI_Command_Definition_t torqueDemandMaxCommandDefinition =
{
    "maxTorque",
    "maxTorque <maxTorque>:\r\n  Set max torque demand (Nm)\r\n",
    torqueDemandMaxCommand,
    1 /* Number of parameters */
};

BaseType_t speedLimitCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * speedMaxString = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    uint64_t speedLimitMax;
    sscanf(speedMaxString, "%llu", &speedLimitMax);

    setForwardSpeedLimit(speedLimitMax);

    COMMAND_OUTPUT("Setting max speed to %llu (rpm)\n", speedLimitMax);

    return pdFALSE;
}
static const CLI_Command_Definition_t speedLimitCommandDefinition =
{
    "speedLimit",
    "speedLimit <maxSpeed>:\r\n  Set max speed (rpm)\r\n",
    speedLimitCommand,
    1 /* Number of parameters */
};

BaseType_t mcInitCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    HAL_StatusTypeDef rc = mcInit();
    if (rc != HAL_OK) {
        ERROR_PRINT("Failed to start motor controllers\n");
        return rc;
    }

    COMMAND_OUTPUT("MCs Inited\n");

    return pdFALSE;
}
static const CLI_Command_Definition_t mcInitCommandDefinition =
{
    "mcInit",
    "mcInit :\r\n  Start motor controllers\r\n",
    mcInitCommand,
    0 /* Number of parameters */
};

BaseType_t invParamReadCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char *addressString = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    int address;
    if (addressString == NULL || sscanf(addressString, "%i", &address) != 1) {
        COMMAND_OUTPUT("Invalid inverter parameter address\r\n");
        return pdFALSE;
    }

    if (address < 0 || address > 0xFFFF) {
        COMMAND_OUTPUT("Invalid inverter parameter address\r\n");
        return pdFALSE;
    }

    if (mcReadParamCommand((uint16_t)address, 0) != HAL_OK) {
        COMMAND_OUTPUT("Failed to send inverter parameter read\r\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("Sent inverter parameter read: addr=%u (0x%04X)\r\n",
                   (unsigned int)address,
                   (unsigned int)address);
    return pdFALSE;
}

static const CLI_Command_Definition_t invParamReadCommandDefinition =
{
    "invParamRead",
    "invParamRead <addr>:\r\n  Read CM200DZ parameter addr. addr may be decimal or hex.\r\n",
    invParamReadCommand,
    1 /* Number of parameters */
};

BaseType_t invParamWriteCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char *addressString = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    const char *dataString = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);

    int address;
    int data;
    if (addressString == NULL || sscanf(addressString, "%i", &address) != 1) {
        COMMAND_OUTPUT("Invalid inverter parameter address\r\n");
        return pdFALSE;
    }
    if (dataString == NULL || sscanf(dataString, "%i", &data) != 1) {
        COMMAND_OUTPUT("Invalid inverter parameter data\r\n");
        return pdFALSE;
    }

    if (address < 0 || address > 0xFFFF) {
        COMMAND_OUTPUT("Invalid inverter parameter address\r\n");
        return pdFALSE;
    }

    if (data < 0 || data > 0xFFFF) {
        COMMAND_OUTPUT("Invalid inverter parameter data\r\n");
        return pdFALSE;
    }

    if (mcWriteParamCommand((uint16_t)address, (uint16_t)data) != HAL_OK) {
        COMMAND_OUTPUT("Failed to send inverter parameter write\r\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("Sent inverter parameter write: addr=%u (0x%04X), data=%u (0x%04X)\r\n",
                   (unsigned int)address,
                   (unsigned int)address,
                   (unsigned int)data,
                   (unsigned int)data);
    return pdFALSE;
}

static const CLI_Command_Definition_t invParamWriteCommandDefinition =
{
    "invParamWrite",
    "invParamWrite <addr> <data>:\r\n  Write CM200DZ parameter addr. Values may be decimal or hex.\r\n",
    invParamWriteCommand,
    2 /* Number of parameters */
};

/* Guided bench pipeline. Stimulus comes from the external fixture, never from
 * writes to the live DMA buffer. CLI processing is serial and owns this session.
 * Keep each response below PRINT_QUEUE_STRING_SIZE for UART-over-CAN transport.
 */
#include "bench_test_definitions.h"

typedef enum {
    BENCH_MANUAL,
    BENCH_THROTTLE_A,
    BENCH_THROTTLE_B,
    BENCH_THROTTLE,
    BENCH_BRAKE_POSITION,
    BENCH_BRAKE_PRESSURE,
    BENCH_BRAKE_COMBINED,
    BENCH_STEERING
} BenchMeasurement;

typedef struct {
    const char *name;
    BenchMeasurement measurement;
    unsigned int samples;
} BenchStep;

/* IDs match the test plan and HIL fixture profile. Procedures live in the plan. */
static const BenchStep benchSteps[] = {
    {"CLI", BENCH_MANUAL, 1},
    {"Firmware", BENCH_MANUAL, 1},
    {"Throttle A", BENCH_THROTTLE_A, 3},
    {"Throttle B", BENCH_THROTTLE_B, 3},
    {"Calculated throttle", BENCH_THROTTLE, 3},
    {"Throttle disagreement", BENCH_MANUAL, 1},
    {"Throttle range", BENCH_MANUAL, 1},
    {"Brake position", BENCH_BRAKE_POSITION, 3},
    {"Brake pressure", BENCH_BRAKE_PRESSURE, 3},
    {"Combined brake", BENCH_BRAKE_COMBINED, 2},
    {"Brake/throttle overlap", BENCH_MANUAL, 1},
    {"Steering", BENCH_STEERING, 3},
    {"CAN transmit", BENCH_MANUAL, 1},
    {"CAN receive", BENCH_MANUAL, 1},
    {"Buttons", BENCH_MANUAL, 1},
    {"Enable conditions", BENCH_MANUAL, 1},
    {"Disable/fault", BENCH_MANUAL, 1},
    {"Wheel-speed inputs", BENCH_MANUAL, 1},
    {"Revision channels", BENCH_MANUAL, 1}
};

#define BENCH_STEP_COUNT (sizeof(benchSteps) / sizeof(benchSteps[0]))

static bool benchStarted;
static size_t benchStep;
static unsigned int benchSamples;
/* P = passed, F = failed, S = skipped, zero = pending. */
static char benchResults[BENCH_STEP_COUNT];

/* The CLI owns uartRxQueue while a run is active. Replies are fixture/operator
 * input to this invocation, not additional CLI commands. A bounded wait keeps
 * a disconnected console from leaving the command active indefinitely.
 */
#define BENCH_REPLY_TIMEOUT_MS 120000

static bool benchRunActive;
static bool benchRunStopped;
static unsigned int benchRunPhase;
static float benchPercentTolerance;
static float benchAngleTolerance;

static bool benchParameterEquals(const char *command, unsigned int index, const char *value)
{
    BaseType_t length;
    const char *parameter = FreeRTOS_CLIGetParameter(command, index, &length);
    return parameter != NULL && (size_t)length == strlen(value)
        && strncmp(parameter, value, (size_t)length) == 0;
}

static float benchReadMeasurement(BenchMeasurement measurement, ThrottleStatus_t *status)
{
    /* The filter is updated on reads. Allow its full sample window to settle.
     * Never suspend the inverter, ADC, or watchdog tasks during a bench check.
     */
    float actual = 0;
    for (unsigned int i = 0; i < NUM_MEDIAN_FILTER_SAMPLES; i++) {
        switch (measurement) {
            case BENCH_THROTTLE_A:
                actual = calculate_throttle_percent1((uint16_t)getThrottleAFiltered());
                break;
            case BENCH_THROTTLE_B:
                actual = calculate_throttle_percent2((uint16_t)getThrottleBFiltered());
                break;
            case BENCH_THROTTLE:
                *status = getNewThrottle(&actual);
                break;
            case BENCH_BRAKE_POSITION:
                actual = getBrakePotentiometerPercent();
                break;
            case BENCH_BRAKE_PRESSURE:
                actual = getBrakePressurePercent();
                break;
            case BENCH_BRAKE_COMBINED:
                actual = getBrakePositionPercent();
                break;
            case BENCH_STEERING:
                actual = (float)getSteeringAngle();
                break;
            default:
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(INV_COMMAND_TASK_PERIOD_MS));
    }
    return actual;
}

static bool benchReadReply(char *reply, size_t capacity)
{
    size_t length = 0;
    bool overflow = false;
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout = pdMS_TO_TICKS(BENCH_REPLY_TIMEOUT_MS);
    uint8_t character;
    for (;;) {
        TickType_t elapsed = xTaskGetTickCount() - start;
        if (elapsed >= timeout
            || xQueueReceive(uartRxQueue, &character, timeout - elapsed) != pdTRUE) {
            return false;
        }
        if (character == '\n') {
            reply[length] = '\0';
            return !overflow;
        }
        if (character == '\r') {
            continue;
        }
        if (character == '\b' || character == 0x7f) {
            if (length > 0) {
                length--;
            }
        } else if (length + 1 < capacity) {
            reply[length++] = (char)character;
        } else {
            overflow = true;
        }
    }
}

static float benchExpectedPoint(BenchMeasurement measurement, unsigned int point)
{
    static const float points[][3] = {
        [BENCH_MANUAL] = {0, 0, 0},
        [BENCH_THROTTLE_A] = VCU_BENCH_THROTTLE_A_POINTS,
        [BENCH_THROTTLE_B] = VCU_BENCH_THROTTLE_B_POINTS,
        [BENCH_THROTTLE] = VCU_BENCH_THROTTLE_POINTS,
        [BENCH_BRAKE_POSITION] = VCU_BENCH_BRAKE_POSITION_POINTS,
        [BENCH_BRAKE_PRESSURE] = VCU_BENCH_BRAKE_PRESSURE_POINTS,
        [BENCH_BRAKE_COMBINED] = VCU_BENCH_BRAKE_COMBINED_POINTS,
        [BENCH_STEERING] = VCU_BENCH_STEERING_POINTS
    };
    return points[measurement][point];
}

static BaseType_t benchRunCommand(char *writeBuffer, size_t writeBufferLength,
                                 const char *commandString)
{
    if (!benchRunActive) {
        float percentTolerance = VCU_BENCH_PERCENT_TOLERANCE;
        float angleTolerance = VCU_BENCH_ANGLE_TOLERANCE;
        if (!VCU_BENCH_DEFINITIONS_VALIDATED) {
            COMMAND_OUTPUT("Definitions unvalidated. Fill bench_test_definitions.h after validation.\r\n");
            return pdFALSE;
        }
        if (!isfinite(percentTolerance) || !isfinite(angleTolerance)
            || percentTolerance < 0 || angleTolerance < 0) {
            COMMAND_OUTPUT("ERROR: stored tolerances must be finite and nonnegative.\r\n");
            return pdFALSE;
        }
        benchPercentTolerance = percentTolerance;
        benchAngleTolerance = angleTolerance;
        memset(benchResults, 0, sizeof(benchResults));
        benchStep = 0;
        benchSamples = 0;
        benchStarted = true;
        benchRunActive = true;
        benchRunStopped = false;
        benchRunPhase = 0;
        COMMAND_OUTPUT("RUN T01-T%02u %s. %s. Reply abort to stop.\r\n",
                       (unsigned int)BENCH_STEP_COUNT,
                       "stored limits",
                       VCU_BENCH_DEFINITIONS_REVISION);
        return pdTRUE;
    }
    if (benchRunStopped || benchStep == BENCH_STEP_COUNT) {
        bool passed = true;
        for (size_t i = 0; i < BENCH_STEP_COUNT; i++) {
            passed = passed && benchResults[i] == 'P';
        }
        benchRunActive = false;
        COMMAND_OUTPUT("OVERALL %s. Results retained for vcuTest report.\r\n",
                       passed ? "PASS" : "INCOMPLETE/FAIL");
        return pdFALSE;
    }
    const BenchStep *step = &benchSteps[benchStep];
    switch (benchRunPhase++) {
        case 0:
            COMMAND_OUTPUT("T%02u %s\r\n", (unsigned int)benchStep + 1, step->name);
            return pdTRUE;
        case 1:
            if (benchStep == 1) {
                /* Reuse the handler, never recursively enter the CLI parser. */
                if (versionCLICommand(writeBuffer, writeBufferLength, "version") == pdTRUE) {
                    benchRunPhase = 1;
                }
                return pdTRUE;
            }
            COMMAND_OUTPUT("Follow T%02u in the test plan; complete all subcases.\r\n",
                           (unsigned int)benchStep + 1);
            return pdTRUE;
        case 2:
        case 3:
            benchRunPhase = 4;
            if (step->measurement == BENCH_MANUAL) {
                COMMAND_OUTPUT("Perform subcases; reply read/pass/fail/skip/abort, then Enter.\r\n");
            } else {
                float expected = benchExpectedPoint(step->measurement, benchSamples);
                float tolerance = step->measurement == BENCH_STEERING
                    ? benchAngleTolerance : benchPercentTolerance;
                /* Stable identifiers let the host select a calibrated HIL
                 * recipe. Only acknowledge ready after the HIL has replied. */
                COMMAND_OUTPUT("HIL T%02u P%u/%u E%.9g tol%.4g. Reply ready/skip/abort + Enter.\r\n",
                               (unsigned int)benchStep + 1, benchSamples + 1,
                               step->samples, expected, tolerance);
            }
            return pdTRUE;
        default:
            break;
    }
    char reply[16];
    if (!benchReadReply(reply, sizeof(reply))) {
        benchRunActive = false;
        COMMAND_OUTPUT("STOP T%02u: reply timeout/overflow. Current test pending; use vcuTest report.\r\n",
                       (unsigned int)benchStep + 1);
        return pdFALSE;
    }
    if (strcmp(reply, "abort") == 0) {
        benchRunActive = false;
        COMMAND_OUTPUT("STOP T%02u: aborted. Current test pending; restore fixture and save report.\r\n",
                       (unsigned int)benchStep + 1);
        return pdFALSE;
    }
    char result = 0;
    if (strcmp(reply, "read") == 0 && step->measurement == BENCH_MANUAL) {
        ThrottleStatus_t status = THROTTLE_OK;
        float actual = benchReadMeasurement(BENCH_THROTTLE, &status);
        benchRunPhase = 3;
        COMMAND_OUTPUT("Throttle=%.4g status=%u state=%u (OK=0 DISABLED=1 FAULT=2)\r\n",
                       actual, (unsigned int)status, (unsigned int)fsmGetState(&VCUFsmHandle));
        return pdTRUE;
    } else if (strcmp(reply, "skip") == 0) {
        result = 'S';
    } else if (step->measurement == BENCH_MANUAL) {
        if (strcmp(reply, "pass") == 0) {
            result = 'P';
        } else if (strcmp(reply, "fail") == 0) {
            result = 'F';
        }
    } else if (strcmp(reply, "ready") == 0) {
        float expected = benchExpectedPoint(step->measurement, benchSamples);
        float tolerance = step->measurement == BENCH_STEERING
            ? benchAngleTolerance : benchPercentTolerance;
        ThrottleStatus_t status = THROTTLE_OK;
        float actual = benchReadMeasurement(step->measurement, &status);
        bool passed = isfinite(actual) && fabsf(actual - expected) <= tolerance
            && status == THROTTLE_OK;
        COMMAND_OUTPUT("T%02u point=%u %s actual=%.4g expected=%.4g tol=%.4g status=%u\r\n",
                       (unsigned int)benchStep + 1, benchSamples + 1,
                       passed ? "PASS" : "FAIL", actual, expected, tolerance, (unsigned int)status);
        if (!passed) {
            benchResults[benchStep] = 'F';
            benchRunStopped = true;
        } else if (++benchSamples == step->samples) {
            benchResults[benchStep++] = 'P';
            benchSamples = 0;
            benchRunPhase = 0;
        } else {
            benchRunPhase = 3;
        }
        return pdTRUE;
    }
    if (result == 0) {
        benchRunPhase = 3;
        COMMAND_OUTPUT("Invalid reply; use the choices in the prompt.\r\n");
        return pdTRUE;
    }
    benchResults[benchStep] = result;
    COMMAND_OUTPUT("T%02u operator=%c; retain evidence.\r\n", (unsigned int)benchStep + 1, result);
    if (result == 'F') {
        benchRunStopped = true;
    } else {
        benchStep++;
    }
    benchSamples = 0;
    benchRunPhase = 0;
    return pdTRUE;
}

/* Only run and report are commands. Replies belong to the active run. */
static BaseType_t benchTestCommand(char *writeBuffer, size_t writeBufferLength,
                                  const char *commandString)
{
    static size_t reportLine;
    BaseType_t length;
    if (FreeRTOS_CLIGetParameter(commandString, 2, &length) != NULL) {
        COMMAND_OUTPUT("Use vcuTest run or vcuTest report; limits are saved in definitions.\r\n");
        return pdFALSE;
    }
    if (benchParameterEquals(commandString, 1, "run")) {
        return benchRunCommand(writeBuffer, writeBufferLength, commandString);
    }
    if (benchParameterEquals(commandString, 1, "report")) {
        if (!benchStarted) {
            COMMAND_OUTPUT("No results. Use vcuTest run.\r\n");
            return pdFALSE;
        }
        if (reportLine < BENCH_STEP_COUNT) {
            vTaskDelay(pdMS_TO_TICKS(20));
            COMMAND_OUTPUT("T%02u %c %s\r\n", (unsigned int)reportLine + 1,
                           benchResults[reportLine] ? benchResults[reportLine] : '-',
                           benchSteps[reportLine].name);
            reportLine++;
            return pdTRUE;
        }
        bool allPassed = true;
        for (size_t i = 0; i < BENCH_STEP_COUNT; i++) {
            allPassed = allPassed && benchResults[i] == 'P';
        }
        reportLine = 0;
        COMMAND_OUTPUT("OVERALL %s (P=pass F=fail S=skip -=pending)\r\n",
                       allPassed ? "PASS" : "INCOMPLETE/FAIL");
        return pdFALSE;
    }
    COMMAND_OUTPUT("vcuTest run | report. During run: ready, read, pass/fail, skip, abort.\r\n");
    return pdFALSE;
}

static const CLI_Command_Definition_t benchTestCommandDefinition = {
    "vcuTest",
    "vcuTest run | report:\r\n Run tests using saved definitions, or print results\r\n",
    benchTestCommand,
    -1 /* Handler validates the command and rejects extra parameters. */
};

HAL_StatusTypeDef stateMachineMockInit()
{
    if (FreeRTOS_CLIRegisterCommand(&benchTestCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&throttleABCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&brakePressureCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&throttleCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&fakeHvToggleCommandDefinition) != pdPASS) {
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
    if (FreeRTOS_CLIRegisterCommand(&debugUartOverCanCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getThrottleABCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getThrottleCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }    
    if (FreeRTOS_CLIRegisterCommand(&getADCInputsCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&torqueDemandMaxCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&speedLimitCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&mcInitCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&invParamReadCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&invParamWriteCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getBrakeCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getSteeringCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }


    return HAL_OK;
}
