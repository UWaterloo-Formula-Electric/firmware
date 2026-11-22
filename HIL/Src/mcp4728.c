/**
  *****************************************************************************
  * @file    mcp4728.c
  * @brief   MCP4728 external quad I2C DAC driver
  * @details Uses the MCP4728 Multi-Write command (datasheet Section 5.6.2) in
  * three steps, matching the onboarding i2c_dac driver flow:
  *   1. buildOutputFrame: Packs the 3-byte Multi-Write frame using named
  *      command and configuration bit definitions.
  *   2. transmitFrame: Sends the frame over I2C1 via i2cBus.
  *   3. activateDac: Pulses the target chip's active-low LDAC_x pin to latch
  *      the new code onto VOUT.
  *****************************************************************************
  */

#include "mcp4728.h"

#include <math.h>

#include "bsp.h"
#include "debug.h"
#include "i2cBus.h"

// All four MCP4728 chips sit on I2C1
#define MCP4728_I2C_BUS              I2C_BUS_1

// Factory-default 7-bit address when A2:A0 = 000 is 0x60 (0b1100000).
// If the 4 chips have distinct programmed EEPROM addresses, they sit at 0x60..0x63.
#define MCP4728_BASE_ADDRESS7        0x60U

// Single-channel Multi-Write frame is always 3 bytes long
#define MCP4728_FRAME_LENGTH         3U

// Datasheet Table 1-2 specifies a 210 ns minimum LDAC low pulse width (t_LDAC).
// 50 NOPs at 216 MHz is ~230+ ns and works both before and after FreeRTOS starts.
#define MCP4728_LDAC_PULSE_NOPS      50

/* MCP4728 Multi-Write Command Bits (Datasheet Section 5.6.2 & Table 4-3) ---- */

// Byte 0: [C2 | C1 | C0 | W1 | W0 | DAC1 | DAC0 | UDAC]
// C2:C0 = 010 and W1:W0 = 00 select the Multi-Write command (writes to the
// volatile DAC input register without wearing out EEPROM).
#define MCP4728_C2                   0U
#define MCP4728_C1                   1U
#define MCP4728_C0                   0U
#define MCP4728_W1                   0U
#define MCP4728_W0                   0U

// UDAC = 1 defers updating the analog output (VOUT) until the chip's physical
// active-low LDAC pin is pulsed low.
#define MCP4728_UDAC                 1U

// Byte 1 upper nibble: [VREF | PD1 | PD0 | GX | D11 | D10 | D9 | D8]
// VREF = 0 selects external VDD (3.3V) as the voltage reference.
// PD1:PD0 = 00 selects normal operation (not powered down).
// GX = 0 selects 1x internal gain (ignored when VREF = 0).
#define MCP4728_VREF                 0U
#define MCP4728_PD1                  0U
#define MCP4728_PD0                  0U
#define MCP4728_GX                   0U

static HAL_StatusTypeDef buildOutputFrame(Mcp4728Channel_t channel, uint16_t dacCode,
                                          uint8_t frame[MCP4728_FRAME_LENGTH])
{
    if (channel >= MCP4728_NUM_CHANNELS || dacCode > MCP4728_MAX_CODE || frame == NULL) {
        return HAL_ERROR;
    }

    // Channel select bits DAC1 (bit 2) and DAC0 (bit 1) select VOUTA..VOUTD (0..3)
    uint8_t dac1 = ((uint8_t)channel >> 1U) & 0x01U;
    uint8_t dac0 = (uint8_t)channel & 0x01U;

    // Byte 0: Command type (C2:C0 = 010, W1:W0 = 00), channel (DAC1:DAC0), and UDAC bit
    frame[0] = (MCP4728_C2 << 7U)
             | (MCP4728_C1 << 6U)
             | (MCP4728_C0 << 5U)
             | (MCP4728_W1 << 4U)
             | (MCP4728_W0 << 3U)
             | (dac1 << 2U)
             | (dac0 << 1U)
             | MCP4728_UDAC;

    // Byte 1: Reference, power-down, internal gain, and upper 4 code bits (D11:D8)
    frame[1] = (MCP4728_VREF << 7U)
             | (MCP4728_PD1 << 6U)
             | (MCP4728_PD0 << 5U)
             | (MCP4728_GX << 4U)
             | ((dacCode >> 8U) & 0x0FU);

    // Byte 2: Lower 8 code bits (D7:D0)
    frame[2] = (uint8_t)(dacCode & 0xFFU);

    return HAL_OK;
}

static HAL_StatusTypeDef transmitFrame(Mcp4728Chip_t chip, uint8_t frame[MCP4728_FRAME_LENGTH])
{
    // i2cBus expects the 7-bit address shifted left by 1 bit for the R/W bit
    uint16_t chipAddress = (uint16_t)((MCP4728_BASE_ADDRESS7 + (uint8_t)chip) << 1U);

    if (i2cWrite(MCP4728_I2C_BUS, chipAddress, frame, MCP4728_FRAME_LENGTH) == HAL_OK) {
        return HAL_OK;
    }

    // Factory-fresh MCP4728 chips all share address 0x60 until their EEPROM
    // address bits are programmed. Because UDAC = 1 defers output latching to
    // the chip's dedicated LDAC_x pin, we can fall back to 0x60 and let
    // activateDac() latch only the target chip.
    if (chip != MCP4728_CHIP_1) {
        uint16_t defaultAddress = (uint16_t)(MCP4728_BASE_ADDRESS7 << 1U);
        if (i2cWrite(MCP4728_I2C_BUS, defaultAddress, frame, MCP4728_FRAME_LENGTH) == HAL_OK) {
            return HAL_OK;
        }
    }

    ERROR_PRINT("Failed to transmit frame to MCP4728 chip %d\n", chip + 1);
    return HAL_ERROR;
}

static void holdLdacLow(void)
{
    for (volatile int i = 0; i < MCP4728_LDAC_PULSE_NOPS; i++) {
        __NOP();
    }
}

static HAL_StatusTypeDef activateDac(Mcp4728Chip_t chip)
{
    // LDAC is active-low: pull the target chip's LDAC pin low to transfer the
    // input register into the DAC output register, then park it high again.
    switch (chip) {
        case MCP4728_CHIP_1:
            LDAC_1_LOW;
            holdLdacLow();
            LDAC_1_HIGH;
            return HAL_OK;
        case MCP4728_CHIP_2:
            LDAC_2_LOW;
            holdLdacLow();
            LDAC_2_HIGH;
            return HAL_OK;
        case MCP4728_CHIP_3:
            LDAC_3_LOW;
            holdLdacLow();
            LDAC_3_HIGH;
            return HAL_OK;
        case MCP4728_CHIP_4:
            LDAC_4_LOW;
            holdLdacLow();
            LDAC_4_HIGH;
            return HAL_OK;
        default:
            return HAL_ERROR;
    }
}

HAL_StatusTypeDef mcp4728Init(void)
{
    // CubeMX initializes LDAC_1..4 low at reset. A low LDAC makes the DAC
    // latch immediately on every I2C byte ACK, defeating the UDAC bit. Park
    // all four LDAC pins high before the first I2C transfer.
    LDAC_1_HIGH;
    LDAC_2_HIGH;
    LDAC_3_HIGH;
    LDAC_4_HIGH;

    for (int i = 0; i < MCP4728_NUM_CHIPS; i++) {
        uint8_t address7 = (uint8_t)(MCP4728_BASE_ADDRESS7 + i);
        if (i2cIsDeviceReady(MCP4728_I2C_BUS, (uint16_t)(address7 << 1U)) != HAL_OK) {
            DEBUG_PRINT("MCP4728 chip %d (0x%02X) did not ACK on I2C1\n", i + 1, address7);
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef mcp4728SetChannelRaw(Mcp4728Chip_t chip, Mcp4728Channel_t channel, uint16_t dacCode)
{
    uint8_t frame[MCP4728_FRAME_LENGTH];

    if (chip >= MCP4728_NUM_CHIPS || channel >= MCP4728_NUM_CHANNELS) {
        ERROR_PRINT("Failed to set MCP4728, bad chip %d or channel %d\n", chip, channel);
        return HAL_ERROR;
    }

    if (buildOutputFrame(channel, dacCode, frame) != HAL_OK) {
        ERROR_PRINT("Failed to build MCP4728 frame for code %u\n", dacCode);
        return HAL_ERROR;
    }

    if (transmitFrame(chip, frame) != HAL_OK) {
        return HAL_ERROR;
    }

    return activateDac(chip);
}

HAL_StatusTypeDef mcp4728SetChannelVoltageMv(Mcp4728Chip_t chip, Mcp4728Channel_t channel, float millivolts, float gain)
{
    if (gain <= 0.0f) {
        gain = MCP4728_GAIN_UNITY;
    }

    // Divide the desired connector output voltage by the external op-amp gain
    // (1.5x on DAC_OUT14..18, 1.0x on DAC_OUT3..13) to get the required MCP4728 pin voltage
    float dacPinMv = millivolts / gain;

    if (dacPinMv < 0.0f || dacPinMv > MCP4728_VDD_MV) {
        ERROR_PRINT("Target %.1f mV (gain %.2f) requires %.1f mV at DAC pin (max %.0f mV)\n", millivolts, gain, dacPinMv, MCP4728_VDD_MV);
        return HAL_ERROR;
    }

    // Map V_dac in [0, 3300 mV] onto the 12-bit code range [0, 4095]
    uint16_t code = (uint16_t)roundf((dacPinMv / MCP4728_VDD_MV) * (float)MCP4728_MAX_CODE);
    if (code > MCP4728_MAX_CODE) {
        code = MCP4728_MAX_CODE;
    }

    return mcp4728SetChannelRaw(chip, channel, code);
}
