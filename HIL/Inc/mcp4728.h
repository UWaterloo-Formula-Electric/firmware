/**
  *****************************************************************************
  * @file    mcp4728.h
  * @brief   MCP4728 external quad I2C DAC driver
  * @details Four MCP4728-E/UN 12-bit quad DACs sit on I2C1, each with its own
  * active-low hardware latch pin (LDAC_1..LDAC_4 defined in bsp.h):
  *   - DAC_OUT3..13 (Chips 1-3) are buffered at 1.0x gain (0 to 3.30V).
  *   - DAC_OUT14..18 (Chip 3 Ch D and Chip 4 Ch A-D) pass through a
  *     non-inverting op-amp stage (Vin_ShiftUp, G = 1 + 10k/20k = 1.5x)
  *     powered from the 5V rail, giving a 0 to 4.95V output range.
  *****************************************************************************
  */

#ifndef MCP4728_H
#define MCP4728_H

#include "stm32f7xx_hal.h"

typedef enum {
    MCP4728_CHIP_1 = 0,
    MCP4728_CHIP_2,
    MCP4728_CHIP_3,
    MCP4728_CHIP_4,
    MCP4728_NUM_CHIPS
} Mcp4728Chip_t;

typedef enum {
    MCP4728_CHANNEL_A = 0, // VOUTA (DAC1:DAC0 = 00)
    MCP4728_CHANNEL_B,     // VOUTB (DAC1:DAC0 = 01)
    MCP4728_CHANNEL_C,     // VOUTC (DAC1:DAC0 = 10)
    MCP4728_CHANNEL_D,     // VOUTD (DAC1:DAC0 = 11)
    MCP4728_NUM_CHANNELS
} Mcp4728Channel_t;

// 12-bit DAC: D11:D0 spans 0 to 4095
#define MCP4728_CODE_COUNT    4096U
#define MCP4728_MAX_CODE      (MCP4728_CODE_COUNT - 1U)

// Full-scale DAC reference voltage when VREF = 0 (VDD = 3.3V = 3300 mV)
#define MCP4728_VDD_MV        3300.0f

// External op-amp gain stages on the HIL board
#define MCP4728_GAIN_UNITY    1.0f // Direct 3.3V outputs (DAC_OUT3..13)
#define MCP4728_GAIN_5V       1.5f // 5V shifted outputs (DAC_OUT14..18, G = 1 + 10k/20k)

// Parks LDAC_1..4 high and probes I2C1 for the four DAC chips
HAL_StatusTypeDef mcp4728Init(void);

// Writes a raw 12-bit code (0 to 4095) to one MCP4728 channel and pulses LDAC
HAL_StatusTypeDef mcp4728SetChannelRaw(Mcp4728Chip_t chip, Mcp4728Channel_t channel, uint16_t dacCode);

// Converts a desired connector output voltage (in mV) into a DAC code (dividing
// by the external op-amp gain first) and updates the channel
HAL_StatusTypeDef mcp4728SetChannelVoltageMv(Mcp4728Chip_t chip, Mcp4728Channel_t channel,
                                              float millivolts, float gain);

#endif /* MCP4728_H */
