// Copyright @ 2015 Waterloo Hybrid

#ifndef LTC6811_H

#define LTC6811_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f7xx_hal.h"

// --- Private Defines ---
// The following defines change depending on battery box layout
// TODO: Update these to 2018 values
#define NUM_BOARDS                  1   // Number of AMS boards in system
#define CELLS_PER_BOARD             12  // Number of valid cells per board, starting from the most negative terminal
#define THERMISTORS_PER_BOARD       12   // Number of thermistors attached to each AMS, starting from A0

// Public defines
#define NUM_VOLTAGE_CELLS           (NUM_BOARDS*CELLS_PER_BOARD)
#define NUM_TEMP_CELLS              (NUM_BOARDS*THERMISTORS_PER_BOARD)

/* Public Functions */
HAL_StatusTypeDef batt_read_cell_voltages_and_temps(float *cell_voltage_array, float *cell_temp_array);


HAL_StatusTypeDef batt_balance_cell(int cell);
bool batt_is_cell_balancing(int cell);
HAL_StatusTypeDef batt_unset_balancing_all_cells();
HAL_StatusTypeDef batt_write_balancing_config();
HAL_StatusTypeDef batt_test_bus();

HAL_StatusTypeDef batt_init();

#endif /* end of include guard: LTC6811_H */
