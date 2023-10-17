/* Owen Brake - May 2021
 * This is an interface which all the ltc chip drivers will use as a sort of prototype
 * The idea is that we are somewhat often changing LTC chip version so this is a way to just abstract away some things
 * The current implemented chips are: LTC6812, LTC6804
 * */

#ifndef LTC_CHIP_INTERFACE_H
#define LTC_CHIP_INTERFACE_H
#include "ltc_common.h"
#include "ltc_chip.h"

typedef enum ltc_command_t {
	ADCV = 0,
	ADAX,
	ADOW_UP,
	ADOW_DOWN,
	ADSTAT,
} ltc_command_t;

typedef enum voltage_operation_t {
	OPEN_WIRE = 0,
	POLL_VOLTAGE = 1,
} voltage_operation_t;

typedef struct open_wire_failure_t {
	bool occurred: 1;
	uint8_t num_times_consec : 2;
} open_wire_failure_t;

void batt_init_chip_configs(void);
HAL_StatusTypeDef batt_write_config(void);
HAL_StatusTypeDef batt_verify_config(void);
HAL_StatusTypeDef batt_check_stat_A(void);
HAL_StatusTypeDef batt_readBackCellVoltage(float *cell_voltage_array, voltage_operation_t voltage_operation);
void batt_set_temp_config(size_t channel);
HAL_StatusTypeDef batt_broadcast_command(ltc_command_t curr_command); 
HAL_StatusTypeDef batt_read_thermistors(size_t channel, float *cell_temp_array);
void batt_set_balancing_cell (int board, int chip, int cell);
void batt_unset_balancing_cell (int board, int chip, int cell);
bool batt_get_balancing_cell_state(int board, int chip, int cell);
HAL_StatusTypeDef batt_config_discharge_timer(DischargeTimerLength length);


extern open_wire_failure_t open_wire_failure[NUM_BOARDS*CELLS_PER_BOARD];

#endif
