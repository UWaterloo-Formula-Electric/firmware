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
#include "beaglebone.h"
#include "traction_control.h"

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
    COMMAND_OUTPUT("Brake %f\n", brake);

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

BaseType_t getADCInputs(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    DEBUG_PRINT("Throttle A %u (ADC: %lu), B %u (ADC: %lu)\n",
                   calculate_throttle_percent1(brakeThrottleSteeringADCVals[THROTTLE_A_INDEX]),
                   brakeThrottleSteeringADCVals[THROTTLE_A_INDEX],
                   calculate_throttle_percent2(brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]),
                   brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]);
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

BaseType_t setTcKp(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    float tmpKp = 0.0f;
    sscanf(param, "%f", &tmpKp);
    tc_kP = tmpKp;

    COMMAND_OUTPUT("Setting kP %f\n", tc_kP);
    return pdFALSE;
}
static const CLI_Command_Definition_t setTcKpCommandDefinition =
{
    "setTcKp",
    "setTcKp <kP>:\r\n set TC kP value\r\n",
    setTcKp,
    1 /* Number of parameters */
};

BaseType_t setTcAbsErrorFloor(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    sscanf(param, "%f", &tc_error_floor_rad_s);

    COMMAND_OUTPUT("setting error floor rad/s %f\n", tc_error_floor_rad_s);
    return pdFALSE;
}
static const CLI_Command_Definition_t setTcAbsErrorFloorCommandDefinition =
{
    "setTcAbsErrorFloor",
    "setTcAbsErrorFloor <errorFloor>:\r\n set TC error floor value (0,inf)\r\n",
    setTcAbsErrorFloor,
    1 /* Number of parameters */
};

BaseType_t setTcTorqueDemandFloor(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    float new_tc_torque_max_floor = 0.0f;

    sscanf(param, "%f", &new_tc_torque_max_floor);
    
    if (new_tc_torque_max_floor <= 0 || new_tc_torque_max_floor > MAX_TORQUE_DEMAND_DEFAULT)
    {
        COMMAND_OUTPUT("invalid torque demand floor. Must be (0,30]. You gave: %f\n", new_tc_torque_max_floor);
    }
    else
    {
        tc_torque_max_floor = new_tc_torque_max_floor;
        COMMAND_OUTPUT("set error floor %f\n", tc_torque_max_floor);
    }
    return pdFALSE;
}
static const CLI_Command_Definition_t setTcTorqueDemandFloorCommandDefinition =
{
    "setTcTorqueDemandFloor",
    "setTcTorqueDemandFloor <torqueDemand>:\r\n set TC torque demand floor value (0,30]\r\n",
    setTcTorqueDemandFloor,
    1 /* Number of parameters */
};

BaseType_t setTcMinPercentError(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    float new_tc_min_percent_error = 0.0f;

    sscanf(param, "%f", &new_tc_min_percent_error);
    
    if (new_tc_min_percent_error <= 0 || new_tc_min_percent_error > MAX_TORQUE_DEMAND_DEFAULT)
    {
        COMMAND_OUTPUT("invalid torque demand floor. Must be (0,30]. You gave: %f\n", new_tc_min_percent_error);
    }
    else
    {
        tc_min_percent_error = new_tc_min_percent_error;
        COMMAND_OUTPUT("set error floor %f\n", tc_min_percent_error);
    }
    return pdFALSE;
}
static const CLI_Command_Definition_t setTcMinPercentErrorCommandDefinition =
{
    "setTcMinPercentError",
    "setTcMinPercentError <percentError>:\r\n set TC torque demand floor value [0,1]\r\n",
    setTcMinPercentError,
    1 /* Number of parameters */
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
    uint8_t index = fsmGetState(&fsmHandle);
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
BaseType_t beagleBonePower(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * onOffParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    bool onOff = false;
    if (STR_EQ(onOffParam, "on", paramLen)) {
        onOff = true;
    } else if (STR_EQ(onOffParam, "off", paramLen)) {
        onOff = false;
    } else {
        COMMAND_OUTPUT("Unkown parameter\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("Turning BeagleBone %s\n", onOff?"on":"off");
    beaglebonePower(onOff);

    return pdFALSE;
}
static const CLI_Command_Definition_t beagleBonePowerCommandDefinition =
{
    "BB",
    "BB <on|off>:\r\n  sets the power state for the BeagleBone\r\n",
    beagleBonePower,
    1 /* Number of parameters */
};

BaseType_t torqueDemandMaxCommand(char *writeBuffer, size_t writeBufferLength,
                       const char *commandString)
{
    BaseType_t paramLen;
    const char * torqueMaxString = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    uint64_t maxTorqueDemand;
    sscanf(torqueMaxString, "%llu", &maxTorqueDemand);

    if(maxTorqueDemand > 30){
        COMMAND_OUTPUT("Max torque input out of range, must be between 0 and 30");
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
    if (FreeRTOS_CLIRegisterCommand(&setTcKpCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&setTcAbsErrorFloorCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&setTcTorqueDemandFloorCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&setTcMinPercentErrorCommandDefinition) != pdPASS) {
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
    if (FreeRTOS_CLIRegisterCommand(&beagleBonePowerCommandDefinition) != pdPASS) {
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
    if (FreeRTOS_CLIRegisterCommand(&getBrakeCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    if (FreeRTOS_CLIRegisterCommand(&getSteeringCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }


    return HAL_OK;
}
