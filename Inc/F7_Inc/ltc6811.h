// Copyright @ 2015 Waterloo Hybrid

#ifndef LTC6811_H

#define LTC6811_H

#include <stdint.h>

// The following defines change depending on battery box layout
#define NUM_BOARDS                  5   // Number of AMS boards in system
#define CELLS_PER_BOARD             12  // Number of valid cells per board, starting from the most negative terminal
#define THERMISTORS_PER_BOARD       8   // Number of thermistors attached to each AMS, starting from A0

// The following defines are always fixed due to AMS architecture, DO NOT CHANGE
#define VOLTAGE_BLOCKS_PER_BOARD    4   // Number of voltage blocks per AMS board
#define VOLTAGES_PER_BLOCK          3   // Number of voltage reading per block
#define VOLTAGE_MEASURE_DELAY_MS    5   // Length of time for voltage measurements to finish
#define TEMP_MEASURE_DELAY_MS       5   // Length of time for temperature measurements to finish

/* Public Functions */
HAL_StatusTypeDef batt_read_cell_voltages_and_temps(float *cell_voltage_array, float *cell_temp_array);
HAL_StatusTypeDef batt_init();

#endif /* end of include guard: LTC6811_H */
