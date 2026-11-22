/**
  *****************************************************************************
  * @file    bspdSim.h
  * @brief   BSPD fault simulation for the 26_BMU interlock loop
  * @details Simulates the two analog sensor inputs feeding the Brake System
  * Plausibility Device (BSPD.SchDoc) on the 26_BMU board:
  *   1. BRAKE_IN (driven by STM32 internal DAC_1 on J9A Pin 1, 0 - 3.30V)
  *   2. HALL_SENSE (driven by MCP4728 Chip 3 Ch D on J9B Pin 12, 0 - 4.95V)
  *
  * Hardware Comparator Thresholds on 26_BMU (5V rail):
  *   - Brake Open-Circuit (U7B):   Trips if BRAKE_IN < 0.238V (5V * 1k / 21k)
  *   - Brake Short-Circuit (U7A):  Trips if BRAKE_IN > 4.762V (5V * 20k / 21k)
  *   - Brake Plausibility (U10A):  Trips if BRAKE_IN > 2.500V (R61=10k, R66=10k)
  *   - Current Plausibility (U10B):Trips if HALL_SENSE > 4.004V (R48=1k, R56=4.02k)
  *   - Relay Latch Delay:          > 500 ms concurrent brake + current trip
  *****************************************************************************
  */

#ifndef HIL_BSPD_SIM_H
#define HIL_BSPD_SIM_H

#include "stm32f7xx_hal.h"

// Brake sensor voltage levels (Volts) on J9A Pin 1 (DAC_1)
#define BSPD_BRAKE_IDLE_V         0.50f   // Safe unpressed idle (> 0.238V OC, < 2.50V trip)
#define BSPD_BRAKE_THRESH_V       2.50f   // U10A trip threshold: 5V * 10k / (10k + 10k)
#define BSPD_BRAKE_ACTIVE_V       2.80f   // Simulated hard braking (> 2.50V threshold)
#define BSPD_BRAKE_OC_FAULT_V     0.10f   // Simulated open-circuit wire fault (< 0.238V)
#define BSPD_BRAKE_SC_FAULT_V     4.85f   // Simulated short-circuit fault (> 4.762V)

// HV current sensor voltage levels (Volts) on J9B Pin 12 (DAC_OUT14)
#define BSPD_CURRENT_IDLE_V       0.50f   // Nominal idle current (< 4.004V trip)
#define BSPD_CURRENT_THRESH_V     4.004f  // U10B trip threshold: 5V * 4.02k / (1k + 4.02k)
#define BSPD_CURRENT_TRIP_V       4.20f   // Simulated high HV current (> 4.004V threshold)

// Default fault hold time (ms) to exceed the BMU's > 500 ms hardware delay timer
#define BSPD_FAULT_DELAY_MS       750U

// Initializes the external DACs and sets both BSPD channels to safe idle (0.50V)
HAL_StatusTypeDef bspdSimInit(void);

// Sets the simulated brake pressure sensor voltage (0.0V to 3.3V) on J9A Pin 1
HAL_StatusTypeDef bspdSimSetBrakeVoltage(float voltage_V);

// Sets the simulated HV current sensor voltage (0.0V to 4.95V) on J9B Pin 12
HAL_StatusTypeDef bspdSimSetCurrentVoltage(float voltage_V);

// Returns both brake and current outputs to their 0.50V safe idle state
HAL_StatusTypeDef bspdSimResetIdle(void);

// Drives brake (2.80V) and current (4.20V) into trip for duration_ms, then resets to idle
HAL_StatusTypeDef bspdSimInjectFault(uint32_t duration_ms);

// Drops BRAKE_IN to 0.10V (< 0.238V) to trigger an immediate open-circuit BSPD fault
HAL_StatusTypeDef bspdSimInjectOpenCircuit(void);

#endif /* HIL_BSPD_SIM_H */
