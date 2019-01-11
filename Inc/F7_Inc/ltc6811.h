// Copyright @ 2015 Waterloo Hybrid

#ifndef LTC6811_H

#define LTC6811_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f7xx_hal.h"

/* Public Functions */
HAL_StatusTypeDef batt_read_cell_voltages_and_temps(float *cell_voltage_array, float *cell_temp_array);


HAL_StatusTypeDef batt_balance_cell(int cell);
bool batt_is_cell_balancing(int cell);
HAL_StatusTypeDef batt_unset_balancing_all_cells();
HAL_StatusTypeDef batt_write_balancing_config();

HAL_StatusTypeDef batt_init();

#endif /* end of include guard: LTC6811_H */
