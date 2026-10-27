/**
  *****************************************************************************
  * @file    bspdSim.c
  * @brief   BSPD fault simulation for the 26_BMU interlock loop
  * @details Maps the logical BSPD brake and current sensor voltages to their
  * physical HIL connectors:
  *   - BRAKE_IN   -> J9A Pin 1 (DAC_1, STM32 PA4, 1.0x gain, 0 - 3.30V)
  *   - HALL_SENSE -> J9B Pin 12 (DAC_OUT14, MCP4728 Chip 3 Ch D, 1.5x gain, 0 - 4.95V)
  *
  * Note: HALL_SENSE requires > 4.004V to trip comparator U10B on the BMU,
  * which exceeds the 3.3V limit of the STM32 internal DAC. Using DAC_OUT14
  * leverages the 1.5x op-amp stage on the HIL board to reach 4.20V.
  *****************************************************************************
  */

#include "bspdSim.h"

#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "dacBus.h"
#include "debug.h"
#include "mcp4728.h"

#define MV_PER_VOLT               1000.0f

// Hardware channel routing on the HIL board
#define BSPD_BRAKE_DAC_CHANNEL    DAC_1_CHANNEL
#define BSPD_CURRENT_MCP_CHIP     MCP4728_CHIP_3
#define BSPD_CURRENT_MCP_CHANNEL  MCP4728_CHANNEL_D
#define BSPD_CURRENT_AMP_GAIN     MCP4728_GAIN_5V

static void delayMs(uint32_t ms)
{
    // Use FreeRTOS delay if the scheduler is running, otherwise use HAL_Delay
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    } else {
        HAL_Delay(ms);
    }
}

HAL_StatusTypeDef bspdSimInit(void)
{
    // Initialize MCP4728 DACs 
    if (mcp4728Init() != HAL_OK) {
        ERROR_PRINT("Failed to initialize MCP4728 DACs\n");
        return HAL_ERROR;
    }

    // Drive both signals to 0.50V immediately so BRAKE_IN stays inside the
    // valid window (0.238V to 2.50V) and does not trip an open-circuit fault
    return bspdSimResetIdle();
}

HAL_StatusTypeDef bspdSimSetBrakeVoltage(float voltage_V)
{
    // Set the brake voltage to the specified voltage
    return dacBusSetVoltageMv(BSPD_BRAKE_DAC_CHANNEL, voltage_V * MV_PER_VOLT);
}

HAL_StatusTypeDef bspdSimSetCurrentVoltage(float voltage_V)
{
    // Set the current sensor voltage to the specified voltage
    return mcp4728SetChannelVoltageMv(BSPD_CURRENT_MCP_CHIP, BSPD_CURRENT_MCP_CHANNEL, voltage_V * MV_PER_VOLT, BSPD_CURRENT_AMP_GAIN);
}

HAL_StatusTypeDef bspdSimResetIdle(void)
{
    // Reset both the brake and current sensor voltages to idle values
    HAL_StatusTypeDef brakeStatus = bspdSimSetBrakeVoltage(BSPD_BRAKE_IDLE_V);
    HAL_StatusTypeDef currentStatus = bspdSimSetCurrentVoltage(BSPD_CURRENT_IDLE_V);

    if (brakeStatus != HAL_OK || currentStatus != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef bspdSimInjectFault(uint32_t duration_ms)
{
    // Set fault duration to default if not specified
    if (duration_ms == 0U) {
        duration_ms = BSPD_FAULT_DELAY_MS;
    }

    // 1. Assert hard braking (2.80V > 2.50V threshold on U10A -> BRAKE_TRIP high)
    if (bspdSimSetBrakeVoltage(BSPD_BRAKE_ACTIVE_V) != HAL_OK) {
        return HAL_ERROR;
    }

    // 2. Assert high HV current (4.20V > 4.004V threshold on U10B -> CURR_TRIP high)
    if (bspdSimSetCurrentVoltage(BSPD_CURRENT_TRIP_V) != HAL_OK) {
        return HAL_ERROR;
    }

    // 3. Hold both fault conditions past the BMU's > 500 ms delay timer so
    //    relay K2 de-energizes and opens the shutdown interlock loop
    delayMs(duration_ms);

    // 4. Return analog outputs to safe idle (relay K2 remains latched open in hardware)
    return bspdSimResetIdle();
}

HAL_StatusTypeDef bspdSimInjectOpenCircuit(void)
{
    // Drive BRAKE_IN below 0.238V (to 0.10V) so window comparator U7B flags an
    // open-circuit wire fault
    return bspdSimSetBrakeVoltage(BSPD_BRAKE_OC_FAULT_V);
}
