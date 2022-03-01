
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

    if (batt_readBackCellVoltage(cell_voltage_array) != HAL_OK)
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

	if (batt_readBackCellVoltage(cell_voltages_single_reading) != HAL_OK)
	{
		return HAL_ERROR;
	}

	// Copy our cell voltage readings to the array
	addCellVoltages(cell_voltages_single_reading, cell_voltages);

    return HAL_OK;
}

HAL_StatusTypeDef checkForOpenCircuit()
{
    // Perform averaging of multiple voltage readings to account for potential
    // bad connections to AMS boards that causes noise
    float cell_voltages_pullup[CELLS_PER_BOARD * NUM_BOARDS] = {0};
    float cell_voltages_pulldown[CELLS_PER_BOARD * NUM_BOARDS] = {0};

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

    for (int board = 0; board < NUM_BOARDS; board++)
    {
        for (int cell = 1; cell < CELLS_PER_BOARD; cell++)
        {
            float pullup = cell_voltages_pullup[board*CELLS_PER_BOARD + cell];
            float pulldown = cell_voltages_pulldown[board*CELLS_PER_BOARD + cell];

            if (float_abs(pullup - pulldown) > (0.4))
            {
                ERROR_PRINT("Cell %d open (PU: %f, PD: %f, diff: %f > 0.4)\n",
                            (cell+board*CELLS_PER_BOARD), pullup, pulldown,
                            float_abs(pullup - pulldown));
                return HAL_ERROR;
            }
        }

		// First cell in board
        if (float_abs(cell_voltages_pullup[board*CELLS_PER_BOARD] - 0) < 0.0002) {
                ERROR_PRINT("Cell %d open (val: %f, diff: %f > 0.0002)\n",
                            board*CELLS_PER_BOARD, cell_voltages_pullup[board*CELLS_PER_BOARD],
                            float_abs(cell_voltages_pullup[board*CELLS_PER_BOARD] - 0));
                return HAL_ERROR;
        }
		// Last cell in board
		size_t last_cell_in_board = board*CELLS_PER_BOARD + CELLS_PER_BOARD-1;
        if (float_abs(cell_voltages_pullup[last_cell_in_board] - 0) < 0.0002) {
                ERROR_PRINT("Cell %d open (val: %f, diff: %f > 0.0002)\n",
                            last_cell_in_board, cell_voltages_pullup[last_cell_in_board],
                            float_abs(cell_voltages_pullup[last_cell_in_board] - 0));
                return HAL_ERROR;
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


