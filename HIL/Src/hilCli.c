/**
  *****************************************************************************
  * @file    hilCli.c
  * @brief   HIL's own CLI commands
  * @details Raw bus pokes for bench bring-up: scan an I2C bus, and read or
  * write a single device register. These are deliberately device agnostic, so
  * a chip can be exercised before any driver exists for it. A driver for a
  * specific part should add its own higher level commands here, next to these.
  * Also exposes direct duty cycle control for the PWM_8/9/10 bench channels.
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
#include "debug.h"
#include "i2cBus.h"
#include "pwmBus.h"

// Valid 7 bit range, excluding the reserved blocks at each end
#define I2C_SCAN_FIRST_ADDRESS 0x08
#define I2C_SCAN_LAST_ADDRESS 0x77

#define PWM_FIRST_CHANNEL 8
#define PWM_LAST_CHANNEL 10

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

    if (FreeRTOS_CLIRegisterCommand(&pwmSetDutyCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    if (FreeRTOS_CLIRegisterCommand(&pwmStopCommandDefinition) != pdPASS) {
        return HAL_ERROR;
    }

    return HAL_OK;
}
