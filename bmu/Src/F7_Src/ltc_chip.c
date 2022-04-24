
#include "ltc_chip.h"
#include "bsp.h"
#include "math.h"
#include "stdbool.h"
#include "errorHandler.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include "ltc_chip_interface.h"
#include "batteries.h"

#define OPEN_WIRE_IBUS_TOLERANCE_A (10.0f)


HAL_StatusTypeDef batt_init()
{

    batt_init_chip_configs();

    if (batt_spi_wakeup(true) != HAL_OK) {
        ERROR_PRINT("Failed to wake up boards\n");
        return HAL_ERROR;
    }

    if(batt_write_config() != HAL_OK) {
    	ERROR_PRINT("Failed to write batt config to boards\n");
        return HAL_ERROR;
    }

	if(batt_verify_config() != HAL_OK){
		ERROR_PRINT("Failed to read batt config from boards\n");
		return HAL_ERROR;
	}

    return HAL_OK;
}

HAL_StatusTypeDef batt_read_cell_voltages(float *cell_voltage_array)
{
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }
	
	batt_broadcast_command(ADCV);

    vTaskDelay(VOLTAGE_MEASURE_DELAY_MS);
    delay_us(VOLTAGE_MEASURE_DELAY_EXTRA_US);
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    if (batt_readBackCellVoltage(cell_voltage_array, POLL_VOLTAGE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_read_cell_temps_single_channel(size_t channel, float *cell_temp_array)
{

    if (batt_spi_wakeup(true /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    // Validate parameters
    if(c_assert(channel < TEMP_CHANNELS_PER_BOARD))
    {
        return HAL_ERROR;
    }

	batt_set_temp_config(channel);
    if (batt_write_config() != HAL_OK)
    {
        ERROR_PRINT("Failed to setup mux for temp reading\n");
        return HAL_ERROR;
    }

    delay_us(MUX_MEASURE_DELAY_US);
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

	batt_broadcast_command(ADAX);
	
    delay_us(TEMP_MEASURE_DELAY_US);
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    if (batt_read_thermistors(channel, cell_temp_array) != HAL_OK)
    {
        ERROR_PRINT("Failed to read temp adc vals\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_read_cell_temps(float *cell_temp_array)
{
	static uint8_t curr_channel = 0;
	for (int i = 0; i < NUM_THERMISTOR_MEASUREMENTS_PER_CYCLE; i++)
	{
		if (batt_read_cell_temps_single_channel(curr_channel, cell_temp_array) != HAL_OK)
		{
			return HAL_ERROR;
		}
		curr_channel++;
		if(curr_channel >= THERMISTORS_PER_BOARD)
		{
			curr_channel = 0;
		}
	}

    return HAL_OK;
}

HAL_StatusTypeDef batt_read_cell_voltages_and_temps(float *cell_voltage_array, float *cell_temp_array){
    if (batt_read_cell_voltages(cell_voltage_array) != HAL_OK) {
        ERROR_PRINT("Failed to read cell voltages\n");
        return HAL_ERROR;
    }
    if (batt_read_cell_temps(cell_temp_array) != HAL_OK) {
        ERROR_PRINT("Failed to read cell temperatures\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

float float_abs(float x)
{
    if (x<0) {
        return (-1*x);
    } else {
        return x;
    }
}

void addCellVoltages(float *cell_voltages, float *cell_voltages_average)
{
    for (int i = 0; i < CELLS_PER_BOARD * NUM_BOARDS; i++) {
        cell_voltages_average[i] += cell_voltages[i];
    }
}

void divideCellVoltages(float *cell_voltages_average, unsigned int num_readings)
{
    for (int i = 0; i < CELLS_PER_BOARD * NUM_BOARDS; i++) {
        cell_voltages_average[i] /= num_readings;
    }
}

// Arrays to use with open wire test
float cell_voltages_single_reading[CELLS_PER_BOARD * NUM_BOARDS];

// Perform open wire test cell voltage reading, either pullup or pulldown
// Average num_readings voltages to account for noise
HAL_StatusTypeDef performOpenCircuitTestReading(float *cell_voltages, bool pullup,
                                                unsigned int num_readings)
{
    if (num_readings <= 0) {
        return HAL_ERROR;
    }

    for (int i = 0; i < num_readings; i++) {
        if (batt_spi_wakeup(false /* not sleeping*/)) {
            return HAL_ERROR;
        }

        if (batt_broadcast_command(pullup ? ADOW_UP : ADOW_DOWN) != HAL_OK) {
            return HAL_ERROR;
        }

        vTaskDelay(VOLTAGE_MEASURE_DELAY_MS);
        delay_us(VOLTAGE_MEASURE_DELAY_EXTRA_US);
    }

	if (batt_spi_wakeup(false /* not sleeping*/))
	{
		return HAL_ERROR;
	}

	if (batt_readBackCellVoltage(cell_voltages_single_reading, OPEN_WIRE) != HAL_OK)
	{
		return HAL_ERROR;
	}

	addCellVoltages(cell_voltages_single_reading, cell_voltages);

    return HAL_OK;
}

HAL_StatusTypeDef checkForOpenCircuit()
{
    // Perform averaging of multiple voltage readings to account for potential
    // bad connections to AMS boards that causes noise
    float cell_voltages_pullup[CELLS_PER_BOARD * NUM_BOARDS] = {0};
    float cell_voltages_pulldown[CELLS_PER_BOARD * NUM_BOARDS] = {0};
	
	float last_IBus = 0.0f;
	// If we can't get it from the IBus queue, skip the check
	bool skip_IBus_check = getIBus(&last_IBus) != HAL_OK;

    if (performOpenCircuitTestReading(cell_voltages_pullup, true /* pullup */,
                                      NUM_OPEN_WIRE_TEST_VOLTAGE_READINGS)
        != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (performOpenCircuitTestReading(cell_voltages_pulldown, false /* pullup */,
                                      NUM_OPEN_WIRE_TEST_VOLTAGE_READINGS)
        != HAL_OK)
    {
        return HAL_ERROR;
    }

    float curr_IBus = 0.0f;
	skip_IBus_check |= getIBus(&curr_IBus) != HAL_OK;
	if (!skip_IBus_check)
	{
		float IBus_error = float_abs(last_IBus - curr_IBus);
		if (IBus_error > OPEN_WIRE_IBUS_TOLERANCE_A)
		{
			// Open Wire check only really works if IBus is constant
			DEBUG_PRINT("Sharp IBus spike over > %f A, at %f A\n", OPEN_WIRE_IBUS_TOLERANCE_A, IBus_error);
			return HAL_OK;
		}
	}

    for (int board = 0; board < NUM_BOARDS; board++)
    {
        for (int cell = 1; cell < CELLS_PER_BOARD; cell++)
        {
        	uint8_t cellIdx = board * CELLS_PER_BOARD + cell;
        	if(!open_wire_failure[cellIdx].occurred)
			{
				float pullup = cell_voltages_pullup[cellIdx];
				float pulldown = cell_voltages_pulldown[cellIdx];
				
				if (float_abs(pullup - pulldown) > (0.4))
				{
					ERROR_PRINT("Cell %d open (PU: %f, PD: %f, diff: %f > 0.4)\n",
								cellIdx, pullup, pulldown,
								float_abs(pullup - pulldown));
					return HAL_ERROR;
				}
				if(cell == CELLS_PER_BOARD - 1 && (float_abs(cell_voltages_pulldown[cellIdx] - 0) < 0.0002))
				{	
					ERROR_PRINT("Cell %d open (val: %f, diff: %f < 0.0002)\n",
								cellIdx, cell_voltages_pulldown[cellIdx],
								float_abs(cell_voltages_pulldown[cellIdx] - 0));
					return HAL_ERROR;
				}
			}
			else // PEC mismatch on this cell
			{
				open_wire_failure[cellIdx].occurred = false;
			}
        }


		// First cell in board
		uint8_t first_cell_idx = board*CELLS_PER_BOARD;
        if (!open_wire_failure[first_cell_idx].occurred && (float_abs(cell_voltages_pullup[first_cell_idx] - 0) < 0.0002)) {
                ERROR_PRINT("Cell %d open (val: %f, diff: %f < 0.0002)\n",
                            first_cell_idx, cell_voltages_pullup[first_cell_idx],
                            float_abs(cell_voltages_pullup[first_cell_idx] - 0));
                return HAL_ERROR;
        }
		else if(open_wire_failure[first_cell_idx].occurred) // PEC mismatch on this cell
		{
			open_wire_failure[first_cell_idx].occurred = false;
		}
	
    }

    return HAL_OK;
}

// Need to write config after
HAL_StatusTypeDef batt_balance_cell(int cell)
{
    if (c_assert(cell < NUM_VOLTAGE_CELLS))
    {
        return HAL_ERROR;
    }

    int boardIdx = cell / CELLS_PER_BOARD;
    int chipIdx = (cell % CELLS_PER_BOARD) / CELLS_PER_CHIP;
    int amsCellIdx = cell - (boardIdx * CELLS_PER_BOARD + chipIdx * CELLS_PER_CHIP);

    batt_set_balancing_cell(boardIdx, chipIdx, amsCellIdx);

    return HAL_OK;
}
HAL_StatusTypeDef batt_stop_balance_cell(int cell)
{
    if (c_assert(cell < NUM_VOLTAGE_CELLS))
    {
        return HAL_ERROR;
    }

    int boardIdx = cell / CELLS_PER_BOARD;
    int chipIdx = (cell % CELLS_PER_BOARD) / CELLS_PER_CHIP;
    int amsCellIdx = cell - (boardIdx * CELLS_PER_BOARD + chipIdx * CELLS_PER_CHIP);

    batt_unset_balancing_cell(boardIdx, chipIdx, amsCellIdx);

    return HAL_OK;
}

// Need to read config first
bool batt_is_cell_balancing(int cell)
{
    if (c_assert(cell < NUM_VOLTAGE_CELLS))
    {
        return false;
    }

    int boardIdx = cell / CELLS_PER_BOARD;
    int chipIdx = (cell % CELLS_PER_BOARD) / CELLS_PER_CHIP;
    int amsCellIdx = cell - (boardIdx * CELLS_PER_BOARD + chipIdx * CELLS_PER_CHIP);

    return batt_get_balancing_cell_state(boardIdx, chipIdx, amsCellIdx);
}

HAL_StatusTypeDef batt_unset_balancing_all_cells()
{
    for (int board = 0; board < NUM_BOARDS; board++) {
    	for(int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++) {
			for (int cell = 0; cell < CELLS_PER_CHIP; cell++) {
				batt_unset_balancing_cell(board, chip, cell);
			}
		}
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_set_disharge_timer(DischargeTimerLength length)
{
    if (length >= INVALID_DT_TIME) {
        return HAL_ERROR;
    }
	
    return batt_config_discharge_timer(length);

}

HAL_StatusTypeDef balanceTest()
{
    if (batt_spi_wakeup(true) != HAL_OK)
    {
        return HAL_ERROR;
    }

    batt_init_chip_configs();

    if (batt_balance_cell(0) != HAL_OK) {
        return HAL_ERROR;
    }

    batt_set_disharge_timer(DT_30_SEC);

    if (batt_write_config() != HAL_OK)
    {
        return HAL_ERROR;
    }

    vTaskDelay(40000);
    if (batt_unset_balancing_all_cells() != HAL_OK) {
        return HAL_ERROR;
    }

    if (batt_spi_wakeup(true) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (batt_write_config() != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}


