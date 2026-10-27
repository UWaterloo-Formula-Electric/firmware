/**
  *****************************************************************************
  * @file    hilCli.c
  * @brief   HIL's own CLI commands
  * @details Raw bus pokes for bench bring-up: scan an I2C bus, and read or
  * write a single device register. These are deliberately device agnostic, so
  * a chip can be exercised before any driver exists for it. A driver for a
  * specific part should add its own higher level commands here, next to these.
  * Also exposes init, duty cycle and stop control for the PWM_8/9/10 bench
  * channels, raw DAC channel control, and BSPD fault simulation commands.
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

#include <ctype.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_CLI.h"
#include "bsp.h"
#include "bspdSim.h"
#include "dacBus.h"
#include "debug.h"
#include "i2cBus.h"
#include "mcp4728.h"
#include "pwmBus.h"

// Valid 7 bit range, excluding the reserved blocks at each end
#define I2C_SCAN_FIRST_ADDRESS 0x08
#define I2C_SCAN_LAST_ADDRESS 0x77

#define PWM_FIRST_CHANNEL 8
#define PWM_LAST_CHANNEL 10

#define DAC_FIRST_CHANNEL 1
#define DAC_LAST_CHANNEL 2

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

// Ensures that the parameter is a valid floating point number and within the range [min, max]
static HAL_StatusTypeDef getFloatFromParam(const char *param, float min, float max, float *value)
{
    // Ensure parameter string is valid and parse as float
    if (param == NULL || sscanf(param, "%f", value) != 1) {
        return HAL_ERROR;
    }

    // Verify parsed value falls within the allowed [min, max] range
    return (*value >= min && *value <= max) ? HAL_OK : HAL_ERROR;
}

// Parses the MCP4728 chip number from a command-line parameter
static HAL_StatusTypeDef getMcpChipFromParam(const char *param, Mcp4728Chip_t *chip)
{
    unsigned int chipNumber = 0;

    // Check for NULL and parse 1-based chip number as an unsigned integer
    if (param == NULL || sscanf(param, "%u", &chipNumber) != 1) {
        return HAL_ERROR;
    }

    // Validate that chip number is in the range [1, MCP4728_NUM_CHIPS] (1..4)
    if (chipNumber < 1 || chipNumber > MCP4728_NUM_CHIPS) {
        return HAL_ERROR;
    }

    // Convert 1-based chip number (1..4) to 0-based enum (MCP4728_CHIP_1..4)
    *chip = (Mcp4728Chip_t)(chipNumber - 1);
    return HAL_OK;
}

// Parses an MCP4728 channel identifier ('A', 'B', 'C', or 'D') from CLI.
static HAL_StatusTypeDef getMcpChannelFromParam(const char *param, Mcp4728Channel_t *channel)
{
    char ch = 0;

    // Check for NULL and read the first non-whitespace character
    if (param == NULL || sscanf(param, " %c", &ch) != 1) {
        return HAL_ERROR;
    }

    // Normalize to uppercase for case-insensitive input (e.g. 'a' -> 'A')
    ch = (char)toupper((unsigned char)ch);
    if (ch < 'A' || ch > 'D') {
        return HAL_ERROR;
    }

    // Convert 'A'..'D' to 0-based enum (MCP4728_CHANNEL_A..D)
    *channel = (Mcp4728Channel_t)(ch - 'A');
    return HAL_OK;
}

/* I2C Bench Commands ------------------------------------------------------ */
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

/* PWM Bench Commands ------------------------------------------------------ */
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

    if (getFloatFromParam(FreeRTOS_CLIGetParameter(commandString, 2, &paramLen), 0.0f, 100.0f, &dutyPercent) != HAL_OK) {
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

/* GPIO Bench Commands ----------------------------------------------------- */
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

/* Internal & External DAC Commands ---------------------------------------- */

/**
 * @brief  CLI handler to set an internal STM32 DAC channel voltage in millivolts.
 * @note   Syntax: dacSet <1|2> <millivolts>
 *         Channel 1 drives PA4 (Connector J9A Pin 1, BRAKE_IN).
 *         Channel 2 drives PA5 (Connector J9A Pin 3)
 */
static BaseType_t dacSetCommand(char *writeBuffer, size_t writeBufferLength, const char *commandString)
{
    BaseType_t paramLen;
    unsigned int channelNum = 0;
    float millivolts = 0.0f;
    const char *channelParam = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);
    const char *mvParam = FreeRTOS_CLIGetParameter(commandString, 2, &paramLen);

    // 1. Parse and validate the internal DAC channel number (must be 1 or 2)
    if (channelParam == NULL || sscanf(channelParam, "%u", &channelNum) != 1 ||
        channelNum < DAC_FIRST_CHANNEL || channelNum > DAC_LAST_CHANNEL) {
        COMMAND_OUTPUT("Channel must be %d or %d\n", DAC_FIRST_CHANNEL, DAC_LAST_CHANNEL);
        return pdFALSE;
    }

    // 2. Parse and validate the target voltage in millivolts (0 to 3300 mV)
    if (getFloatFromParam(mvParam, 0.0f, DAC_BUS_VREF_MV, &millivolts) != HAL_OK) {
        COMMAND_OUTPUT("Voltage must be between 0 and %.0f mV\n", DAC_BUS_VREF_MV);
        return pdFALSE;
    }

    // 3. Set the internal DAC output voltage via the dacBus driver
    if (dacBusSetVoltageMv(channelNum, millivolts) != HAL_OK) {
        COMMAND_OUTPUT("Failed to set DAC_%u\n", channelNum);
        return pdFALSE;
    }

    // 4. Output confirmation to user
    COMMAND_OUTPUT("Set DAC_%u to %.2f mV\n", channelNum, millivolts);
    return pdFALSE;
}

static const CLI_Command_Definition_t dacSetCommandDefinition =
{
    "dacSet",
    "dacSet <1|2> <millivolts>:\r\n  Set internal DAC channel (1 or 2) voltage in mV (0-3300)\r\n",
    dacSetCommand,
    2 /* Number of parameters */
};

/**
 * @brief  CLI handler to set an external MCP4728 12-bit I2C DAC channel voltage.
 * @note   Syntax: mcpSet <chip 1..4> <ch A..D> <millivolts> [gain]
 *         Optional gain defaults to 1.0x (channels DAC_OUT3..13, 0-3.3V range).
 *         Channels DAC_OUT14..18 have an onboard 1.5x op-amp stage (0-4.95V range).
 */
static BaseType_t mcpSetCommand(char *writeBuffer, size_t writeBufferLength, const char *commandString)
{
    BaseType_t paramLen;
    Mcp4728Chip_t chip;
    Mcp4728Channel_t channel;
    float millivolts = 0.0f;
    float gain = MCP4728_GAIN_UNITY;

    // 1. Parse and validate MCP4728 chip number (1 to 4)
    if (getMcpChipFromParam(FreeRTOS_CLIGetParameter(commandString, 1, &paramLen), &chip) != HAL_OK) {
        COMMAND_OUTPUT("Chip must be 1 to %d\n", MCP4728_NUM_CHIPS);
        return pdFALSE;
    }

    // 2. Parse and validate channel ('A', 'B', 'C', or 'D')
    if (getMcpChannelFromParam(FreeRTOS_CLIGetParameter(commandString, 2, &paramLen), &channel) != HAL_OK) {
        COMMAND_OUTPUT("Channel must be A, B, C, or D\n");
        return pdFALSE;
    }

    // 3. Parse optional 4th parameter specifying external op-amp gain (default 1.0x)
    const char *gainParam = FreeRTOS_CLIGetParameter(commandString, 4, &paramLen);
    if (gainParam != NULL && getFloatFromParam(gainParam, 0.1f, 10.0f, &gain) != HAL_OK) {
        COMMAND_OUTPUT("Gain must be positive\n");
        return pdFALSE;
    }

    // 4. Parse and validate target voltage in mV (up to 3300 mV * gain)
    if (getFloatFromParam(FreeRTOS_CLIGetParameter(commandString, 3, &paramLen),
                          0.0f, MCP4728_VDD_MV * gain, &millivolts) != HAL_OK) {
        COMMAND_OUTPUT("Voltage must be between 0 and %.0f mV\n", MCP4728_VDD_MV * gain);
        return pdFALSE;
    }

    // 5. Transmit multi-write frame over I2C and pulse hardware LDAC latch pin
    if (mcp4728SetChannelVoltageMv(chip, channel, millivolts, gain) != HAL_OK) {
        COMMAND_OUTPUT("Failed to set MCP4728 chip %d channel %c\n", chip + 1, 'A' + channel);
        return pdFALSE;
    }

    // 6. Output confirmation to user
    COMMAND_OUTPUT("Set MCP4728 Chip %d Ch %c to %.2f mV (gain %.2f)\n",
                   chip + 1, 'A' + channel, millivolts, gain);
    return pdFALSE;
}

static const CLI_Command_Definition_t mcpSetCommandDefinition =
{
    "mcpSet",
    "mcpSet <chip 1..4> <ch A..D> <millivolts> [gain]:\r\n  Set MCP4728 external DAC channel voltage in mV\r\n",
    mcpSetCommand,
    -1 /* Variable parameters: 3 or 4 */
};

/* BSPD Simulation Commands ------------------------------------------------ */

/**
 * @brief  CLI handler to set the simulated brake pressure sensor voltage.
 * @note   Syntax: bspdSetBrake <volts>
 *         Drives STM32 internal DAC_1 (PA4) routed to J9A Pin 1 (BRAKE_IN).
 *         BMU thresholds: < 0.238V (open-circuit), > 2.50V (active braking)
 */
static BaseType_t bspdSetBrakeCommand(char *writeBuffer, size_t writeBufferLength,
                                      const char *commandString)
{
    BaseType_t paramLen;
    float volts = 0.0f;
    float maxVolts = DAC_BUS_VREF_MV / 1000.0f;

    // 1. Parse and validate voltage argument in Volts (0.0 to 3.3V)
    if (getFloatFromParam(FreeRTOS_CLIGetParameter(commandString, 1, &paramLen),
                          0.0f, maxVolts, &volts) != HAL_OK) {
        COMMAND_OUTPUT("Voltage must be between 0.0 and %.1f V\n", maxVolts);
        return pdFALSE;
    }

    // 2. Drive simulated brake sensor voltage via bspdSim driver
    if (bspdSimSetBrakeVoltage(volts) != HAL_OK) {
        COMMAND_OUTPUT("Failed to set brake sensor voltage\n");
        return pdFALSE;
    }

    // 3. Output confirmation to user
    COMMAND_OUTPUT("Brake sensor set to %.3f V (J9A Pin 1)\n", volts);
    return pdFALSE;
}

static const CLI_Command_Definition_t bspdSetBrakeCommandDefinition =
{
    "bspdSetBrake",
    "bspdSetBrake <volts>:\r\n  Set simulated brake pressure sensor voltage (0.0-3.3V) on J9A Pin 1\r\n",
    bspdSetBrakeCommand,
    1 /* Number of parameters */
};

/**
 * @brief  CLI handler to set the simulated HV hall current sensor voltage.
 * @note   Syntax: bspdSetCurrent <volts>
 *         Drives MCP4728 Chip 3 Channel D on J9B Pin 12 (HALL_SENSE) through
 *         the 1.5x non-inverting op-amp stage (0.0 - 4.95V range).
 *         BMU threshold: > 4.004V indicates HV current trip (> 5kW at 400V)
 */
static BaseType_t bspdSetCurrentCommand(char *writeBuffer, size_t writeBufferLength,
                                        const char *commandString)
{
    BaseType_t paramLen;
    float volts = 0.0f;
    float maxVolts = (MCP4728_VDD_MV * MCP4728_GAIN_5V) / 1000.0f;

    // 1. Parse and validate voltage argument in Volts (0.0 to 4.95V)
    if (getFloatFromParam(FreeRTOS_CLIGetParameter(commandString, 1, &paramLen),
                          0.0f, maxVolts, &volts) != HAL_OK) {
        COMMAND_OUTPUT("Voltage must be between 0.0 and %.2f V\n", maxVolts);
        return pdFALSE;
    }

    // 2. Drive simulated current sensor voltage via bspdSim driver
    if (bspdSimSetCurrentVoltage(volts) != HAL_OK) {
        COMMAND_OUTPUT("Failed to set current sensor voltage\n");
        return pdFALSE;
    }

    // 3. Output confirmation to user
    COMMAND_OUTPUT("Current sensor set to %.3f V (J9B Pin 12)\n", volts);
    return pdFALSE;
}

static const CLI_Command_Definition_t bspdSetCurrentCommandDefinition =
{
    "bspdSetCurrent",
    "bspdSetCurrent <volts>:\r\n  Set simulated HV hall current sensor voltage (0.0-4.95V) on J9B Pin 12\r\n",
    bspdSetCurrentCommand,
    1 /* Number of parameters */
};

/**
 * @brief  CLI handler to inject a concurrent brake + current BSPD trip fault.
 * @note   Syntax: bspdSimFault [duration_ms]
 *         Simultaneously outputs 2.80V on BRAKE_IN (> 2.50V) and 4.20V on HALL_SENSE
 *         (> 4.004V). If sustained past 500ms, the BMU trips interlock relay K2.
 *         After duration_ms, outputs return to safe idle (0.50V)
 */
static BaseType_t bspdSimFaultCommand(char *writeBuffer, size_t writeBufferLength,
                                      const char *commandString)
{
    BaseType_t paramLen;
    unsigned int duration = BSPD_FAULT_DELAY_MS;
    const char *param = FreeRTOS_CLIGetParameter(commandString, 1, &paramLen);

    // 1. Parse optional fault duration in ms (defaults to BSPD_FAULT_DELAY_MS = 750 ms)
    if (param != NULL && (sscanf(param, "%u", &duration) != 1 || duration == 0)) {
        COMMAND_OUTPUT("Duration must be a positive integer in milliseconds\n");
        return pdFALSE;
    }

    // 2. Inject the fault: asserts trip voltages, holds for duration, then resets to idle
    if (bspdSimInjectFault((uint32_t)duration) != HAL_OK) {
        COMMAND_OUTPUT("Failed to inject BSPD fault\n");
        return pdFALSE;
    }

    // 3. Output confirmation to user with injected voltage and duration details
    COMMAND_OUTPUT("Injected BSPD fault (Brake %.2fV, Current %.2fV) for %u ms, then reset to %.2fV\n",
                   BSPD_BRAKE_ACTIVE_V, BSPD_CURRENT_TRIP_V, duration, BSPD_BRAKE_IDLE_V);
    return pdFALSE;
}

static const CLI_Command_Definition_t bspdSimFaultCommandDefinition =
{
    "bspdSimFault",
    "bspdSimFault [duration_ms]:\r\n  Inject concurrent brake (2.80V > 2.50V) & current (4.20V > 4.004V) trip\r\n",
    bspdSimFaultCommand,
    -1 /* Optional duration parameter */
};

/**
 * @brief  CLI handler to reset both simulated sensor voltages to safe idle (0.50V).
 * @note   Syntax: bspdSimReset
 *         Sets BRAKE_IN to 0.50V (inside valid 0.238V - 2.50V window) and HALL_SENSE
 *         to 0.50V (well below 4.004V trip threshold)
 */
static BaseType_t bspdSimResetCommand(char *writeBuffer, size_t writeBufferLength,
                                      const char *commandString)
{
    (void)commandString;

    // 1. Reset both DAC outputs to safe idle levels
    if (bspdSimResetIdle() != HAL_OK) {
        COMMAND_OUTPUT("Failed to reset BSPD simulation\n");
        return pdFALSE;
    }

    // 2. Output confirmation to user
    COMMAND_OUTPUT("BSPD simulation reset to idle (Brake: %.2fV on J9A P1, Current: %.2fV on J9B P12)\n",
                   BSPD_BRAKE_IDLE_V, BSPD_CURRENT_IDLE_V);
    return pdFALSE;
}

static const CLI_Command_Definition_t bspdSimResetCommandDefinition =
{
    "bspdSimReset",
    "bspdSimReset:\r\n  Reset brake and current simulated voltages to safe idle (0.50V)\r\n",
    bspdSimResetCommand,
    0 /* Number of parameters */
};

/**
 * @brief  CLI handler to inject a brake sensor open-circuit fault.
 * @note   Syntax: bspdSimOC
 *         Drives BRAKE_IN on J9A Pin 1 to 0.10V, dropping below the BMU window
 *         comparator U7B threshold (< 0.238V) to simulate a disconnected wire.
 */
static BaseType_t bspdSimOCCommand(char *writeBuffer, size_t writeBufferLength,
                                   const char *commandString)
{
    (void)commandString;

    // 1. Set BRAKE_IN to 0.10V (< 0.238V window comparator low threshold)
    if (bspdSimInjectOpenCircuit() != HAL_OK) {
        COMMAND_OUTPUT("Failed to inject open-circuit fault\n");
        return pdFALSE;
    }

    // 2. Output confirmation to user
    COMMAND_OUTPUT("Injected Brake Open-Circuit fault (%.2fV on J9A Pin 1)\n", BSPD_BRAKE_OC_FAULT_V);
    return pdFALSE;
}

static const CLI_Command_Definition_t bspdSimOCCommandDefinition =
{
    "bspdSimOC",
    "bspdSimOC:\r\n  Inject brake open-circuit fault (0.10V < 0.238V threshold) on J9A Pin 1\r\n",
    bspdSimOCCommand,
    0 /* Number of parameters */
};

HAL_StatusTypeDef hilCliInit(void)
{
    // List of all command definitions to register with FreeRTOS_CLI
    static const CLI_Command_Definition_t *const commands[] = {
        &i2cScanCommandDefinition,
        &i2cReadCommandDefinition,
        &i2cWriteCommandDefinition,
        &pwmInitCommandDefinition,
        &pwmSetDutyCommandDefinition,
        &pwmStopCommandDefinition,
        &gpio3v1EnableCommandDefinition,
        &dacSetCommandDefinition,
        &mcpSetCommandDefinition,
        &bspdSetBrakeCommandDefinition,
        &bspdSetCurrentCommandDefinition,
        &bspdSimFaultCommandDefinition,
        &bspdSimResetCommandDefinition,
        &bspdSimOCCommandDefinition,
    };

    // Iterate through the command list and register each with FreeRTOS_CLI
    for (size_t i = 0; i < (sizeof(commands) / sizeof(commands[0])); i++) {
        if (FreeRTOS_CLIRegisterCommand(commands[i]) != pdPASS) {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}
