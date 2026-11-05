/**
  *****************************************************************************
  * @file    hilCli.c
  * @brief   HIL's own CLI commands
  * @details Raw bus pokes for bench bring-up: scan an I2C bus, and read or
  * write a single device register. These are deliberately device agnostic, so
  * a chip can be exercised before any driver exists for it. A driver for a
  * specific part should add its own higher level commands here, next to these.
  * Also exposes init, duty cycle and stop control for the PWM_8/9/10 bench
  * channels.
  *
  * Commands are registered with FreeRTOS_CLI by hilCliInit(), called from
  * userInit(). common/Src/debug.c's cliTask then reads the debug UART and
  * dispatches to them.
  *
  * The other boards keep their CLI in controlStateMachine_mock.c because it
  * hangs off their state machine mock. HIL has no state machine, hence the
  * different file name.
  *****************************************************************************
  */

#include "hilCli.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_CLI.h"
#include "bsp.h"
#include "debug.h"
#include "i2cBus.h"
#include "pwmBus.h"
#include "i2cDac.h"
#include "throttleSim.h"

// Valid 7 bit range, excluding the reserved blocks at each end
#define I2C_SCAN_FIRST_ADDRESS 0x08
#define I2C_SCAN_LAST_ADDRESS 0x77

#define PWM_FIRST_CHANNEL 8
#define PWM_LAST_CHANNEL 10
#define THROTTLE_PERCENT_MIN -10.0f
#define THROTTLE_PERCENT_MAX 110.0f

static HAL_StatusTypeDef getThrottlePercentFromParam(const char *param, float *percent)
{
    if (param == NULL || sscanf(param, "%f", percent) != 1) {
        return HAL_ERROR;
    }

    return (*percent >= THROTTLE_PERCENT_MIN && *percent <= THROTTLE_PERCENT_MAX) ? HAL_OK : HAL_ERROR;
}

static HAL_StatusTypeDef getBusFromParam(const char *param, I2cBus_t *bus)
{
    unsigned int busNumber = 0;

    if (param == NULL || sscanf(param, "%u", &busNumber) != 1) {
        return HAL_ERROR;
    }

    if (busNumber < 1 || busNumber > NUM_I2C_BUSES) {
        return HAL_ERROR;
    }

    *bus = (I2cBus_t)(busNumber - 1);
    return HAL_OK;
}

static HAL_StatusTypeDef getHexFromParam(const char *param, unsigned int max, unsigned int *value)
{
    if (param == NULL || sscanf(param, "%x", value) != 1) {
        return HAL_ERROR;
    }

    return (*value <= max) ? HAL_OK : HAL_ERROR;
}

static HAL_StatusTypeDef getPwmChannelFromParam(const char *param, PwmChannel_t *channel)
{
    unsigned int channelNumber = 0;

    if (param == NULL || sscanf(param, "%u", &channelNumber) != 1) {
        return HAL_ERROR;
    }

    if (channelNumber < PWM_FIRST_CHANNEL || channelNumber > PWM_LAST_CHANNEL) {
        return HAL_ERROR;
    }

    *channel = (PwmChannel_t)(channelNumber - PWM_FIRST_CHANNEL);
    return HAL_OK;
}

static HAL_StatusTypeDef getDutyCycleFromParam(const char *param, float *dutyPercent)
{
    if (param == NULL || sscanf(param, "%f", dutyPercent) != 1) {
        return HAL_ERROR;
    }

    return (*dutyPercent >= 0.0f && *dutyPercent <= 100.0f) ? HAL_OK : HAL_ERROR;
}

static BaseType_t i2cScanCommand(char *writeBuffer, size_t writeBufferLength,
                                 const char *commandString)
{
    static unsigned int address = I2C_SCAN_FIRST_ADDRESS;
    BaseType_t paramLen;
    I2cBus_t bus;

    if (getBusFromParam(FreeRTOS_CLIGetParameter(commandString, 1, &paramLen), &bus) != HAL_OK) {
        address = I2C_SCAN_FIRST_ADDRESS;
        COMMAND_OUTPUT("Bus must be between 1 and %d\n", NUM_I2C_BUSES);
        return pdFALSE;
    }

    // One address per invocation, since the CLI output buffer only holds one line
    while (address <= I2C_SCAN_LAST_ADDRESS) {
        unsigned int current = address++;

        if (i2cIsDeviceReady(bus, current << 1) == HAL_OK) {
            COMMAND_OUTPUT("Found device at 0x%02X\n", current);
            return pdTRUE;
        }
    }

    address = I2C_SCAN_FIRST_ADDRESS;
    COMMAND_OUTPUT("Scan complete\n");
    return pdFALSE;
}

static const CLI_Command_Definition_t i2cScanCommandDefinition =
{
    "i2cScan",
    "i2cScan <bus>:\r\n  Scan I2C <bus> (1-3) and list the 7 bit addresses that respond\r\n",
    i2cScanCommand,
    1 /* Number of parameters */
};

static BaseType_t i2cReadCommand(char *writeBuffer, size_t writeBufferLength,
                                 const char *commandString)
{
    BaseType_t paramLen;
    I2cBus_t bus;
    unsigned int address;
    unsigned int reg;
    uint8_t value = 0;

    if (getBusFromParam(FreeRTOS_CLIGetParameter(commandString, 1, &paramLen), &bus) != HAL_OK) {
        COMMAND_OUTPUT("Bus must be between 1 and %d\n", NUM_I2C_BUSES);
        return pdFALSE;
    }

    if (getHexFromParam(FreeRTOS_CLIGetParameter(commandString, 2, &paramLen),
                        I2C_SCAN_LAST_ADDRESS, &address) != HAL_OK) {
        COMMAND_OUTPUT("Address must be a 7 bit hex value, 0x%02X to 0x%02X\n",
                       I2C_SCAN_FIRST_ADDRESS, I2C_SCAN_LAST_ADDRESS);
        return pdFALSE;
    }

    if (getHexFromParam(FreeRTOS_CLIGetParameter(commandString, 3, &paramLen),
                        UINT8_MAX, &reg) != HAL_OK) {
        COMMAND_OUTPUT("Register must be a hex value, 0x00 to 0xFF\n");
        return pdFALSE;
    }

    if (i2cReadReg(bus, address << 1, (uint8_t)reg, &value, 1) != HAL_OK) {
        COMMAND_OUTPUT("Read failed\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("0x%02X\n", value);
    return pdFALSE;
}

static const CLI_Command_Definition_t i2cReadCommandDefinition =
{
    "i2cRead",
    "i2cRead <bus> <addr> <reg>:\r\n  Read one byte. <addr> is the 7 bit address from the datasheet\r\n",
    i2cReadCommand,
    3 /* Number of parameters */
};

static BaseType_t i2cWriteCommand(char *writeBuffer, size_t writeBufferLength,
                                  const char *commandString)
{
    BaseType_t paramLen;
    I2cBus_t bus;
    unsigned int address;
    unsigned int reg;
    unsigned int value;
    uint8_t data;

    if (getBusFromParam(FreeRTOS_CLIGetParameter(commandString, 1, &paramLen), &bus) != HAL_OK) {
        COMMAND_OUTPUT("Bus must be between 1 and %d\n", NUM_I2C_BUSES);
        return pdFALSE;
    }

    if (getHexFromParam(FreeRTOS_CLIGetParameter(commandString, 2, &paramLen),
                        I2C_SCAN_LAST_ADDRESS, &address) != HAL_OK) {
        COMMAND_OUTPUT("Address must be a 7 bit hex value, 0x%02X to 0x%02X\n",
                       I2C_SCAN_FIRST_ADDRESS, I2C_SCAN_LAST_ADDRESS);
        return pdFALSE;
    }

    if (getHexFromParam(FreeRTOS_CLIGetParameter(commandString, 3, &paramLen),
                        UINT8_MAX, &reg) != HAL_OK) {
        COMMAND_OUTPUT("Register must be a hex value, 0x00 to 0xFF\n");
        return pdFALSE;
    }

    if (getHexFromParam(FreeRTOS_CLIGetParameter(commandString, 4, &paramLen),
                        UINT8_MAX, &value) != HAL_OK) {
        COMMAND_OUTPUT("Value must be a hex value, 0x00 to 0xFF\n");
        return pdFALSE;
    }

    data = (uint8_t)value;

    if (i2cWriteReg(bus, address << 1, (uint8_t)reg, &data, 1) != HAL_OK) {
        COMMAND_OUTPUT("Write failed\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("Wrote 0x%02X to register 0x%02X\n", data, reg);
    return pdFALSE;
}

static const CLI_Command_Definition_t i2cWriteCommandDefinition =
{
    "i2cWrite",
    "i2cWrite <bus> <addr> <reg> <val>:\r\n  Write one byte. All values except <bus> are hex\r\n",
    i2cWriteCommand,
    4 /* Number of parameters */
};

static BaseType_t pwmInitCommand(char *writeBuffer, size_t writeBufferLength,
                                 const char *commandString)
{
    BaseType_t paramLen;
    PwmChannel_t channel;

    if (getPwmChannelFromParam(FreeRTOS_CLIGetParameter(commandString, 1, &paramLen), &channel) != HAL_OK) {
        COMMAND_OUTPUT("Channel must be %d to %d\n", PWM_FIRST_CHANNEL, PWM_LAST_CHANNEL);
        return pdFALSE;
    }

    if (pwmInitChannel(channel) != HAL_OK) {
        COMMAND_OUTPUT("Failed to init PWM\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("Started PWM_%u at 0%%\n", (unsigned int)channel + PWM_FIRST_CHANNEL);
    return pdFALSE;
}

static const CLI_Command_Definition_t pwmInitCommandDefinition =
{
    "pwmInit",
    "pwmInit <channel>:\r\n  (Re)start PWM_<channel> (8-10) at 0% duty cycle\r\n",
    pwmInitCommand,
    1 /* Number of parameters */
};

static BaseType_t pwmSetDutyCommand(char *writeBuffer, size_t writeBufferLength,
                                    const char *commandString)
{
    BaseType_t paramLen;
    PwmChannel_t channel;
    float dutyPercent;

    if (getPwmChannelFromParam(FreeRTOS_CLIGetParameter(commandString, 1, &paramLen), &channel) != HAL_OK) {
        COMMAND_OUTPUT("Channel must be %d to %d\n", PWM_FIRST_CHANNEL, PWM_LAST_CHANNEL);
        return pdFALSE;
    }

    if (getDutyCycleFromParam(FreeRTOS_CLIGetParameter(commandString, 2, &paramLen), &dutyPercent) != HAL_OK) {
        COMMAND_OUTPUT("Duty cycle must be 0-100\n");
        return pdFALSE;
    }

    if (pwmSetDutyCycle(channel, dutyPercent) != HAL_OK) {
        COMMAND_OUTPUT("Failed to set duty cycle\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("Set PWM_%u to %.1f%%\n", (unsigned int)channel + PWM_FIRST_CHANNEL, dutyPercent);
    return pdFALSE;
}

static const CLI_Command_Definition_t pwmSetDutyCommandDefinition =
{
    "pwmSetDuty",
    "pwmSetDuty <channel> <percent>:\r\n  Set PWM_<channel> (8-10) to <percent> (0-100) duty cycle\r\n",
    pwmSetDutyCommand,
    2 /* Number of parameters */
};

static BaseType_t pwmStopCommand(char *writeBuffer, size_t writeBufferLength,
                                 const char *commandString)
{
    BaseType_t paramLen;
    PwmChannel_t channel;

    if (getPwmChannelFromParam(FreeRTOS_CLIGetParameter(commandString, 1, &paramLen), &channel) != HAL_OK) {
        COMMAND_OUTPUT("Channel must be %d to %d\n", PWM_FIRST_CHANNEL, PWM_LAST_CHANNEL);
        return pdFALSE;
    }

    if (pwmStop(channel) != HAL_OK) {
        COMMAND_OUTPUT("Failed to stop PWM\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("Stopped PWM_%u\n", (unsigned int)channel + PWM_FIRST_CHANNEL);
    return pdFALSE;
}

static const CLI_Command_Definition_t pwmStopCommandDefinition =
{
    "pwmStop",
    "pwmStop <channel>:\r\n  Stop PWM output on PWM_<channel> (8-10)\r\n",
    pwmStopCommand,
    1 /* Number of parameters */
};

static BaseType_t gpio3v1EnableCommand(char *writeBuffer, size_t writeBufferLength,
                                       const char *commandString)
{
    BaseType_t paramLen;
    unsigned int enable;
    const char *param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    if (param == NULL || sscanf(param, "%u", &enable) != 1 || enable > 1) {
        COMMAND_OUTPUT("Must be 0 (disable) or 1 (enable)\n");
        return pdFALSE;
    }

    if (enable) {
        GPIO3V_1_ENABLE;
    } else {
        GPIO3V_1_DISABLE;
    }

    COMMAND_OUTPUT("GPIO3V_1 (PB12) %s\n", GPIO3V_1_IS_ENABLED ? "enabled" : "disabled");
    return pdFALSE;
}

static const CLI_Command_Definition_t gpio3v1EnableCommandDefinition =
{
    "gpio3v1Enable",
    "gpio3v1Enable <0|1>:\r\n  Drive GPIO3V_1 (PB12) low/high\r\n",
    gpio3v1EnableCommand,
    1 /* Number of parameters */
};

static BaseType_t dacProbeCommand(char *writeBuffer, size_t writeBufferLength,
                                  const char *commandString)
{
    (void)commandString;

    if (i2cDacProbe() != HAL_OK) {
        COMMAND_OUTPUT("No ACK from MCP4728 at 0x60 on I2C1\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("MCP4728 acknowledged\n");
    return pdFALSE;
}

static BaseType_t dacSetCodesCommand(char *writeBuffer, size_t writeBufferLength,
                                     const char *commandString)
{
    BaseType_t paramLen;
    unsigned int codeA;
    unsigned int codeB;

    if (getHexFromParam(FreeRTOS_CLIGetParameter(commandString, 1, &paramLen),
                        I2C_DAC_CODE_MAX, &codeA) != HAL_OK) {
        COMMAND_OUTPUT("Codes must be hex, 0x000 to 0xFFF\n");
        return pdFALSE;
    }

    if (getHexFromParam(FreeRTOS_CLIGetParameter(commandString, 2, &paramLen),
                        I2C_DAC_CODE_MAX, &codeB) != HAL_OK) {
        COMMAND_OUTPUT("Codes must be hex, 0x000 to 0xFFF\n");
        return pdFALSE;
    }

    if (i2cDacSetCodes((uint16_t)codeA, (uint16_t)codeB) != HAL_OK) {
        COMMAND_OUTPUT("DAC write failed\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("Set A=0x%03X B=0x%03X\n", codeA, codeB);
    return pdFALSE;
}

static const CLI_Command_Definition_t dacSetCodesCommandDefinition =
{
    "dacSetCodes",
    "dacSetCodes <codeA> <codeB>:\r\n  Set MCP4728 channels A and B to 12 bit hex codes (000 to FFF)\r\n",
    dacSetCodesCommand,
    2 /* Number of parameters */
};

static const CLI_Command_Definition_t dacProbeCommandDefinition =
{
    "dacProbe",
    "dacProbe:\r\n  Check that the MCP4728 acknowledges on I2C1\r\n",
    dacProbeCommand,
    0 /* Number of parameters */
};

static BaseType_t throttleSetCommand(char *writeBuffer, size_t writeBufferLength,
                                     const char *commandString)
{
    BaseType_t paramLen;
    float percent;
    uint16_t codeA;
    uint16_t codeB;

    if (getThrottlePercentFromParam(FreeRTOS_CLIGetParameter(commandString, 1, &paramLen),
                                    &percent) != HAL_OK) {
        COMMAND_OUTPUT("Percent must be a number, -10 to 110\n");
        return pdFALSE;
    }

    if (throttleSimSetPercents(percent, percent, &codeA, &codeB) != HAL_OK) {
        COMMAND_OUTPUT("DAC write failed\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("Set A=B=%.1f%% (A=0x%03X B=0x%03X)\n", percent, codeA, codeB);
    return pdFALSE;
}

static const CLI_Command_Definition_t throttleSetCommandDefinition =
{
    "throttleSet",
    "throttleSet <percent>:\r\n  Drive both throttle pots to the same pedal percentage\r\n",
    throttleSetCommand,
    1 /* Number of parameters */
};

static BaseType_t throttleSplitCommand(char *writeBuffer, size_t writeBufferLength,
                                       const char *commandString)
{
    BaseType_t paramLen;
    float percentA;
    float percentB;
    uint16_t codeA;
    uint16_t codeB;

    if (getThrottlePercentFromParam(FreeRTOS_CLIGetParameter(commandString, 1, &paramLen),
                                    &percentA) != HAL_OK) {
        COMMAND_OUTPUT("Percent A must be a number, -10 to 110\n");
        return pdFALSE;
    }

    if (getThrottlePercentFromParam(FreeRTOS_CLIGetParameter(commandString, 2, &paramLen),
                                    &percentB) != HAL_OK) {
        COMMAND_OUTPUT("Percent B must be a number, -10 to 110\n");
        return pdFALSE;
    }

    if (throttleSimSetPercents(percentA, percentB, &codeA, &codeB) != HAL_OK) {
        COMMAND_OUTPUT("DAC write failed\n");
        return pdFALSE;
    }

    COMMAND_OUTPUT("Set A=%.1f%% (0x%03X) B=%.1f%% (0x%03X)\n", percentA, codeA, percentB, codeB);
    return pdFALSE;
}

static const CLI_Command_Definition_t throttleSplitCommandDefinition =
{
    "throttleSplit",
    "throttleSplit <percentA> <percentB>:\r\n  Drive pot A and pot B to different pedal percentages\r\n",
    throttleSplitCommand,
    2 /* Number of parameters */
};

HAL_StatusTypeDef hilCliInit(void)
{
    if (FreeRTOS_CLIRegisterCommand(&i2cScanCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    if (FreeRTOS_CLIRegisterCommand(&i2cReadCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    if (FreeRTOS_CLIRegisterCommand(&i2cWriteCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    if (FreeRTOS_CLIRegisterCommand(&pwmInitCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    if (FreeRTOS_CLIRegisterCommand(&pwmSetDutyCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    if (FreeRTOS_CLIRegisterCommand(&pwmStopCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    if (FreeRTOS_CLIRegisterCommand(&gpio3v1EnableCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    if (FreeRTOS_CLIRegisterCommand(&dacProbeCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }
    
    if (FreeRTOS_CLIRegisterCommand(&dacSetCodesCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    if (FreeRTOS_CLIRegisterCommand(&throttleSetCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    if (FreeRTOS_CLIRegisterCommand(&throttleSplitCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    return HAL_OK;
}
