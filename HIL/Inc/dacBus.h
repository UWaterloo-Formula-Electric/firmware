/**
  *****************************************************************************
  * @file    dacBus.h
  * @brief   Generic DAC output wrappers for the STM32 internal DAC channels
  * @details DAC_1 (PA4, J9A Pin 1) and DAC_2 (PA5, J9A Pin 3) are the two
  * 12-bit internal DAC channels broken out to the bench connector. Both
  * channels are unity-gain with a 0 to 3.3V (3300 mV) output range.
  *****************************************************************************
  */

#ifndef DAC_BUS_H
#define DAC_BUS_H

#include "stm32f7xx_hal.h"

// 12-bit DAC: D11:D0 spans 0 to 4095
#define DAC_BUS_CODE_COUNT  4096U
#define DAC_BUS_MAX_CODE    (DAC_BUS_CODE_COUNT - 1U)

// Full-scale reference voltage (VDD = 3.3V = 3300 mV)
#define DAC_BUS_VREF_MV     3300.0f

// Starts DAC_1 (at 500 mV safe idle) and DAC_2 (at 0 mV)
HAL_StatusTypeDef dacBusInit(void);

// Sets a raw 12-bit DAC code (0 to 4095) on channel 1 or 2
HAL_StatusTypeDef dacBusSetRaw(uint32_t channel, uint16_t dacValue);

// Converts millivolts (0 to 3300 mV) to a 12-bit code and updates the DAC
HAL_StatusTypeDef dacBusSetVoltageMv(uint32_t channel, float millivolts);

#endif /* DAC_BUS_H */
