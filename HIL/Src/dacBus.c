/**
  *****************************************************************************
  * @file    dacBus.c
  * @brief   Generic DAC output wrappers for the STM32 internal DAC channels
  * @details DAC_1 (PA4, J9A Pin 1) and DAC_2 (PA5, J9A Pin 3) are driven by
  * the STM32's internal 12-bit DAC peripheral (hdac). Callers can pass either
  * a 1-based channel number (1 or 2) or the BSP channel macro (DAC_1_CHANNEL
  * or DAC_2_CHANNEL).
  *****************************************************************************
  */

#include "dacBus.h"

#include <math.h>

#include "bsp.h"
#include "debug.h"

// Pre-load DAC_1 (J9A Pin 1 / BRAKE_IN) to 500 mV before enabling the output
// buffer so the BMU BSPD window comparator never sees 0V (< 238 mV open-circuit
// fault threshold) during power-up.
#define DAC_1_SAFE_BOOT_MV 500.0f

static HAL_StatusTypeDef getDacHalChannel(uint32_t channel, uint32_t *halChannel)
{
    if (channel == 1U || channel == DAC_1_CHANNEL) {
        *halChannel = DAC_1_CHANNEL;
        return HAL_OK;
    }

    if (channel == 2U || channel == DAC_2_CHANNEL) {
        *halChannel = DAC_2_CHANNEL;
        return HAL_OK;
    }

    return HAL_ERROR;
}

static uint16_t millivoltsToCode(float millivolts)
{
    // Map [0, 3300 mV] linearly onto the 12-bit code range [0, 4095]
    float scaled = (millivolts / DAC_BUS_VREF_MV) * (float)DAC_BUS_MAX_CODE;
    uint16_t code = (uint16_t)roundf(scaled);

    return (code > DAC_BUS_MAX_CODE) ? DAC_BUS_MAX_CODE : code;
}

HAL_StatusTypeDef dacBusInit(void)
{
    uint16_t safeBootCode = millivoltsToCode(DAC_1_SAFE_BOOT_MV);

    // Load the initial holding register values before turning on the DAC buffers
    if (HAL_DAC_SetValue(&DAC_HANDLE, DAC_1_CHANNEL, DAC_ALIGN_12B_R, safeBootCode) != HAL_OK) {
        ERROR_PRINT("Failed to set initial value on DAC_1\n");
        return HAL_ERROR;
    }

    // Set initial output values on both DAC channels before enabling them
    if (HAL_DAC_SetValue(&DAC_HANDLE, DAC_2_CHANNEL, DAC_ALIGN_12B_R, 0U) != HAL_OK) {
        ERROR_PRINT("Failed to set initial value on DAC_2\n");
        return HAL_ERROR;
    }

    // Enable the DAC outputs and start generating voltage
    if (HAL_DAC_Start(&DAC_HANDLE, DAC_1_CHANNEL) != HAL_OK) {
        ERROR_PRINT("Failed to start DAC_1\n");
        return HAL_ERROR;
    }

    // Enable the DAC outputs and start generating voltage
    if (HAL_DAC_Start(&DAC_HANDLE, DAC_2_CHANNEL) != HAL_OK) {
        ERROR_PRINT("Failed to start DAC_2\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef dacBusSetRaw(uint32_t channel, uint16_t dacValue)
{
    uint32_t halChannel;

    // Map the user-facing channel number to the HAL driver's channel definition
    if (getDacHalChannel(channel, &halChannel) != HAL_OK) {
        ERROR_PRINT("Failed to set DAC channel %lu, bad channel\n", (unsigned long)channel);
        return HAL_ERROR;
    }

    // Validate the raw DAC value is within the valid range
    if (dacValue > DAC_BUS_MAX_CODE) {
        ERROR_PRINT("Failed to set DAC channel %lu, code %u out of range\n",
                    (unsigned long)channel, dacValue);
        return HAL_ERROR;
    }

    // Write the raw DAC value to the specified channel
    if (HAL_DAC_SetValue(&DAC_HANDLE, halChannel, DAC_ALIGN_12B_R, dacValue) != HAL_OK) {
        ERROR_PRINT("Failed to write code %u to DAC channel %lu\n",
                    dacValue, (unsigned long)channel);
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef dacBusSetVoltageMv(uint32_t channel, float millivolts)
{
    // Validate the requested voltage is within the DAC's output range
    if (millivolts < 0.0f || millivolts > DAC_BUS_VREF_MV) {
        ERROR_PRINT("Failed to set DAC channel %lu, voltage %.2f mV out of range\n",
                    (unsigned long)channel, millivolts);
        return HAL_ERROR;
    }

    return dacBusSetRaw(channel, millivoltsToCode(millivolts));
}
