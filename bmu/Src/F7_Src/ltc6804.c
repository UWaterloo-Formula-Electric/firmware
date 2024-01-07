/* 
 * Owen Brake - May 2021
 * This is the LTC6804 chip architecture, the current plan is that it uses 
 *
 * */
#include "ltc_chip_interface.h"
#include "string.h"
#if LTC_CHIP == LTC_CHIP_6804

#define WRCFG_BYTE0 0x00
#define WRCFG_BYTE1 0x01

#define RDCFG_BYTE0 0x00
#define RDCFG_BYTE1 0x02

#define RDCVA_BYTE0 0x00
#define RDCVA_BYTE1 0x04

#define RDCVB_BYTE0 0x00
#define RDCVB_BYTE1 0x06

#define RDCVC_BYTE0 0x00
#define RDCVC_BYTE1 0x08

#define RDCVD_BYTE0 0x00
#define RDCVD_BYTE1 0x0a

#define PLADC_BYTE0 0x07
#define PLADC_BYTE1 0x14

#define CLRSTAT_BYTE0 0x07
#define CLRSTAT_BYTE1 0x13

#define CLRCELL_BYTE0 0x07
#define CLRCELL_BYTE1 0x11

#define CLRAUX_BYTE0 0x07
#define CLRAUX_BYTE1 0x12

// For reading auxiliary register group A
#define RDAUXA_BYTE0 0x00
#define RDAUXA_BYTE1 0x0c

// For reading auxiliary register group B
#define RDAUXB_BYTE0 0x00
#define RDAUXB_BYTE1 0x0e

// Use normal MD (7kHz), Discharge not permission, all channels and GPIO 1 & 2
#define ADCVAX_BYTE0 0x05
#define ADCVAX_BYTE1 0x6f

// Use normal MD (7kHz), Discharge not permission, all channels
#define ADCV_BYTE0 0x03
#define ADCV_BYTE1 0x60

// // Use Fast MD (27kHz), Discharge not permission, all channels
// #define ADCV_BYTE0 0x02
// #define ADCV_BYTE1 0xE0

// // Use slow MD (26 Hz), Discharge not permission, all channels
// #define ADCV_BYTE0 0x03
// #define ADCV_BYTE1 0xE0

// Use fast MD (27kHz), Discharge not permission, all channels and GPIO 5
#define ADAX_BYTE0 0x05
#define ADAX_BYTE1 0x65

#define RDSTATA_BYTE0 0x00
#define RDSTATA_BYTE1 0x10

#define RDSTATB_BYTE0 0x00
#define RDSTATB_BYTE1 0x12

// Use normal MD (7kHz), Discharge not permission, all channels
#define ADOW_BYTE0 0x03
#define ADOW_BYTE1(PUP) (0x28 | ((PUP)<<6))

#define ADC_OPT(en) ((en) << 0) // Since we're using the normal 7kHz mode
#define SWTRD(en) ((en) << 1) // We're not using the software time
#define REFON(en) ((en) << 2)

#define BATT_CONFIG_SIZE 6    // Size of Config per Register
#define COMMAND_SIZE 2
#define PEC_SIZE 2
#define VOLTAGE_BLOCK_SIZE 6
#define AUX_BLOCK_SIZE 6
#define CELL_VOLTAGE_SIZE_BYTES 2
#define THERMISTOR_CHIP 0

#define MAX_ADC_CHECKS 10
#define DEBUGGING_AMS

// static const uint8_t LTC_ADDRESS[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD] = {
// 	{0},
//     {1},
//     {2},
//     {3},
//     {4},
//     {5},
//     {6},
//     {7},
//     {8},
//     {9},
//     {10},
//     {11},
//     {12},
//     {13}
// };


static uint8_t cell_voltage_failure[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][VOLTAGE_BLOCKS_PER_CHIP];
open_wire_failure_t open_wire_failure[NUM_BOARDS * CELLS_PER_BOARD];
static uint8_t thermistor_failure[NUM_BOARDS][THERMISTORS_PER_BOARD];


static uint8_t m_batt_config[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};

void batt_init_chip_configs()
{

	memset(cell_voltage_failure, 0, NUM_BOARDS*NUM_LTC_CHIPS_PER_BOARD*VOLTAGE_BLOCKS_PER_CHIP);
	
	memset(thermistor_failure, 0, NUM_BOARDS*THERMISTORS_PER_BOARD);
	memset(open_wire_failure, 0, NUM_BOARDS*CELLS_PER_BOARD*sizeof(open_wire_failure_t));
	for(int board = 0; board < NUM_BOARDS; board++) {
		for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++){
			m_batt_config[board][ltc_chip][0] = REFON(1) | ADC_OPT(0) | SWTRD(1);
		}
	}
}


HAL_StatusTypeDef format_and_send_config(uint8_t config_buffer[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE])
{
	
	const size_t BUFF_SIZE = COMMAND_SIZE + PEC_SIZE + ((BATT_CONFIG_SIZE + PEC_SIZE) * NUM_BOARDS * NUM_LTC_CHIPS_PER_BOARD);
	uint8_t txBuffer[BUFF_SIZE];

	DEBUG_PRINT("***sending config***\n");
	if (batt_format_command(WRCFG_BYTE0, WRCFG_BYTE1, txBuffer) != HAL_OK) {
		ERROR_PRINT("Failed to send write config command\n");
		return HAL_ERROR;
	}

	// Fill data, note that config register is sent in reverse order, as the
	// daisy chain acts as a shift register
	// Therefore, we send data destined for the last board first
	for (int board = (NUM_BOARDS-1); board >= 0; board--) {
		for(int ltc_chip = (NUM_LTC_CHIPS_PER_BOARD-1); ltc_chip >= 0; ltc_chip--){
			// should we remove this ^ for loop or keep it for a future where we increase LTC_CHIPS_PER_BOARD?
			uint32_t configsWritten = ((NUM_BOARDS - 1) - board) * NUM_LTC_CHIPS_PER_BOARD + ((NUM_LTC_CHIPS_PER_BOARD - 1) - ltc_chip);
			size_t startByte = COMMAND_SIZE + PEC_SIZE + configsWritten * (BATT_CONFIG_SIZE + PEC_SIZE);

			for (int dbyte = 0; dbyte < BATT_CONFIG_SIZE; dbyte++)
			{
				txBuffer[startByte + dbyte]
					= config_buffer[board][ltc_chip][dbyte];
			}

			// Data pec
			batt_gen_pec(config_buffer[board][ltc_chip], BATT_CONFIG_SIZE,
						&(txBuffer[startByte + BATT_CONFIG_SIZE]));
		}

	}

	// Send command + data
	if (batt_spi_tx(txBuffer, BUFF_SIZE) != HAL_OK)
	{
		ERROR_PRINT("Failed to transmit config to AMS board\n");
		return HAL_ERROR;
	}

	return HAL_OK;
}

HAL_StatusTypeDef batt_write_config()
{
    return format_and_send_config(m_batt_config);
}

static uint32_t PEC_count = 0;
static uint32_t last_PEC_tick = 0;
static HAL_StatusTypeDef batt_read_data(uint8_t first_byte, uint8_t second_byte, uint8_t* data_buffer, unsigned int response_size){
    const size_t BUFF_SIZE = COMMAND_SIZE + PEC_SIZE + ((response_size + PEC_SIZE) * NUM_BOARDS * NUM_LTC_CHIPS_PER_BOARD);
    const size_t DATA_START_IDX = COMMAND_SIZE + PEC_SIZE;
    uint8_t rxBuffer[BUFF_SIZE];
    uint8_t txBuffer[BUFF_SIZE];

	if (batt_format_command(first_byte, second_byte, txBuffer) != HAL_OK) {
		ERROR_PRINT("Failed to send write config command\n");
		return HAL_ERROR;
	}

	if (spi_tx_rx(txBuffer, rxBuffer, BUFF_SIZE) != HAL_OK) {
		ERROR_PRINT("Failed to send read data command\n");
		return HAL_ERROR;
	}
	// DEBUG_PRINT("Received Data: ");
	// for(int i = 0; i < BUFF_SIZE; i++) {
	// 	DEBUG_PRINT("%x ", rxBuffer[i]);
	// }
	// DEBUG_PRINT("\n");

    for (int board = 0; board < NUM_BOARDS; board++) {
    	for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++){
			int dataStartIdx = DATA_START_IDX + (PEC_SIZE + response_size) * (board * NUM_LTC_CHIPS_PER_BOARD + ltc_chip);
			if (checkPEC(&(rxBuffer[dataStartIdx]), response_size) != HAL_OK)
			{
				ERROR_PRINT("PEC mismatch\n");
				return HAL_ERROR;
			}

			// Copy data out
			for (int i = 0; i < response_size; i++) {
				data_buffer[(board * NUM_LTC_CHIPS_PER_BOARD + ltc_chip)*response_size + i] = rxBuffer[dataStartIdx + i];
			}
		}
    }

	if(xTaskGetTickCount() - last_PEC_tick > 10000)
	{
		// DEBUG_PRINT("\nPEC Rate: %lu errors/10s \n", PEC_count);
		PEC_count = 0;
		last_PEC_tick = xTaskGetTickCount();
	}

	return HAL_OK;
}

HAL_StatusTypeDef batt_read_config(uint8_t config[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE])
{
	if (batt_read_data(RDCFG_BYTE0, RDCFG_BYTE1, (uint8_t*) config, BATT_CONFIG_SIZE) != HAL_OK)
	{
		ERROR_PRINT("Failed to read config\n");
		return HAL_ERROR;
	}

	return HAL_OK;
}

HAL_StatusTypeDef batt_verify_config(){
	uint8_t config_buffer[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};
	if(batt_read_config(config_buffer) != HAL_OK){
		ERROR_PRINT("Failed to read config");
		return HAL_ERROR;
	}
    
    vTaskDelay(T_REFUP_MS);
	
	// Verify was set correctly
	for(int board = 0; board < NUM_BOARDS; board++) {
		for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++) {
			DEBUG_PRINT("Config Read, Board %d, Chip %d: ", board, ltc_chip); 
			for(int buff_byte = 0; buff_byte < BATT_CONFIG_SIZE; buff_byte++) {
				DEBUG_PRINT("0x%x ", config_buffer[board][ltc_chip][buff_byte]);
				if(m_batt_config[board][ltc_chip][buff_byte] != config_buffer[board][ltc_chip][buff_byte]) {
					ERROR_PRINT("\n ERROR: board: %d, ltc_chip: %d, buff_byte %d, mismatch \n", board, ltc_chip, buff_byte);
					return HAL_ERROR;
				}
			}
			DEBUG_PRINT("\n");
		}
	}
	return HAL_OK;
}

HAL_StatusTypeDef batt_send_command(ltc_command_t curr_command) {
    const size_t TX_BUFF_SIZE = COMMAND_SIZE + PEC_SIZE;
    uint8_t txBuffer[TX_BUFF_SIZE];
	
	uint8_t command_byte_low, command_byte_high;
	switch(curr_command) {
		case(ADCV): 
		{
			command_byte_low = ADCV_BYTE0;
			command_byte_high = ADCV_BYTE1;
			break;
		}
		case(ADAX):
		{
			command_byte_low = ADAX_BYTE0;
			command_byte_high = ADAX_BYTE1;
			break;
		}
		case(ADOW_UP):
		{
			command_byte_low = ADOW_BYTE0;
			command_byte_high = ADOW_BYTE1(1);
			break;
		}
		case(ADOW_DOWN):
		{
			command_byte_low = ADOW_BYTE0;
			command_byte_high = ADOW_BYTE1(0);
			break;
		}
		default:
			return HAL_ERROR;
	}

	if (batt_format_command(command_byte_low, command_byte_high, txBuffer) != HAL_OK)
	{
		ERROR_PRINT("Failed to format read voltage command\n");
		return HAL_ERROR;
	}

	if (batt_spi_tx(txBuffer, TX_BUFF_SIZE) != HAL_OK)
	{
		ERROR_PRINT("Failed to transmit read voltage command\n");
		return HAL_ERROR;
	}
	return HAL_OK;
}


HAL_StatusTypeDef batt_broadcast_command(ltc_command_t curr_command) {
	if(batt_send_command(curr_command) != HAL_OK){
		ERROR_PRINT("Failed to send command: %d", curr_command);
		return HAL_ERROR;
	}
	return HAL_OK;
}

/*
 * Read back cell voltages, this assumes that the command to initiate ADC
 * readings has been sent already and the appropriate amount of time has
 * elapsed for readings to finish
 * Used for both batt_read_cell_voltages and open wire check
 */
HAL_StatusTypeDef batt_readBackCellVoltage(float *cell_voltage_array, voltage_operation_t voltage_operation)
{
	if (batt_spi_wakeup(false /* not sleeping*/))
	{
		return HAL_ERROR;
	}

	// Voltage values for one block from all boards
	uint8_t adc_vals[VOLTAGE_BLOCK_SIZE * NUM_BOARDS * NUM_LTC_CHIPS_PER_BOARD] = {0};

	for (int block = 0; block < VOLTAGE_BLOCKS_PER_CHIP; block++) {

		uint8_t cmdByteLow, cmdByteHigh;

		// Select appropriate bank

		switch(block){
			case 0:
			{
				cmdByteLow = RDCVA_BYTE0;
				cmdByteHigh = RDCVA_BYTE1;
				break;
			}
			case 1:
			{
				cmdByteLow = RDCVB_BYTE0;
				cmdByteHigh = RDCVB_BYTE1;
				break;
			}
			case 2:
			{
				cmdByteLow = RDCVC_BYTE0;
				cmdByteHigh = RDCVC_BYTE1;
				break;
			}
			case 3:
			{
				cmdByteLow = RDCVD_BYTE0;
				cmdByteHigh = RDCVD_BYTE1;
				break;
			}
			default:
			{
				ERROR_PRINT("ERROR: Unknown voltage block accessed");
				return HAL_ERROR;
			}
		}

		if (batt_read_data(cmdByteLow, cmdByteHigh, adc_vals, VOLTAGE_BLOCK_SIZE) != HAL_OK) {
			ERROR_PRINT("Failed to read voltage block %d\n", block);
			return HAL_ERROR;
		}

		// Have to read 3 battery voltages from each cell voltage register
		for (int board = 0; board < NUM_BOARDS; board++) {
			for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++){
				for (int cvreg = 0; cvreg < VOLTAGES_PER_BLOCK; cvreg ++)
				{
					uint8_t voltage_terminal = cvreg + block * VOLTAGES_PER_BLOCK;
					if(voltage_terminal == 5)
					{
						continue;
					}

					size_t registerIndex = cvreg*CELL_VOLTAGE_SIZE_BYTES;
					size_t registerStartForChip = (board * NUM_LTC_CHIPS_PER_BOARD + ltc_chip) * (CELL_VOLTAGE_SIZE_BYTES * VOLTAGES_PER_BLOCK);
					size_t index = registerIndex + registerStartForChip;
					size_t cellIdx = cvreg + (board * NUM_LTC_CHIPS_PER_BOARD + ltc_chip) * CELLS_PER_CHIP + VOLTAGES_PER_BLOCK*block;
					uint16_t temp = ((uint16_t) (adc_vals[(index + 1)] << 8 | adc_vals[index]));
					cell_voltage_array[cellIdx] = ((float)temp) / VOLTAGE_REGISTER_COUNTS_PER_VOLT;
				}
			}
		}
	}

    return HAL_OK;
}



void batt_set_temp_config(size_t channel) {
	for (int board = 0; board < NUM_BOARDS; board++)
    {
		uint8_t gpioPins = channel;

		// Set the external MUX to channel we want to read. MUX pin is selected via GPIO2, GPIO3, GPIO4, LSB first.
		m_batt_config[board][THERMISTOR_CHIP][0] = (1<<GPIO5_POS) | ((gpioPins & 0xFF) << GPIO1_POS) | REFON(1) | ADC_OPT(0) | SWTRD(1);
	}
}


HAL_StatusTypeDef batt_read_thermistors(size_t channel, float *cell_temp_array) {
	for(int board = 0; board < NUM_BOARDS; board++) {
			
		// adc values for one block from all boards
		uint8_t adc_vals[AUX_BLOCK_SIZE] = {0};
				
		if(batt_read_data(RDAUXB_BYTE0, RDAUXB_BYTE1, adc_vals, AUX_BLOCK_SIZE) != HAL_OK) {
			DEBUG_PRINT("ERROR: Reading thermistor on board %d, channel %lu failed (perhaps PEC mismatch)\n", board, (unsigned long)channel);
			thermistor_failure[board][channel]++;
			if(thermistor_failure[board][channel] >= NUM_PEC_MISMATCH_CONSECUTIVE_FAILS_ERROR)
			{
				return HAL_ERROR;
			}
			else if(thermistor_failure[board][channel] >= NUM_PEC_MISMATCH_CONSECUTIVE_FAILS_WARNING)
			{
				DEBUG_PRINT("Reached warning for cell thermistor PEC mismatch %u\n", thermistor_failure[board][channel]);
			}
			return HAL_OK;
		}
		size_t cellIdx = (board) * THERMISTORS_PER_BOARD + channel;
		
		// We only use the first GPIO register, 2 bytes out of the total 6 in adc_vals
		uint16_t temp = ((uint16_t) (adc_vals[TEMP_ADC_IDX_HIGH] << 8
									| adc_vals[TEMP_ADC_IDX_LOW]));
		float voltageThermistor = ((float)temp) / VOLTAGE_REGISTER_COUNTS_PER_VOLT;
		cell_temp_array[cellIdx] = batt_convert_voltage_to_temp(voltageThermistor);
		
	}
	return HAL_OK;
}



void batt_set_balancing_cell (int board, int chip, int cell) {
	if(cell >= 5)
	{
		// We skip S6 in the schematic
		cell += 1;
	}
    if (cell < 8) { // 8 bits per byte in the register
        SETBIT(m_batt_config[board][chip][4], cell);
    } else { // This register byte only contains 4 bits
		SETBIT(m_batt_config[board][chip][5], cell - 8);
	}
}


void batt_unset_balancing_cell (int board, int chip, int cell)
{
	if(cell >= 5)
	{
		// We skip S6 in the schematic
		cell += 1;
	}
    if (cell < 8) { // 8 bits per byte in the register
        CLEARBIT(m_batt_config[board][chip][4], cell);
    } else {
        CLEARBIT(m_batt_config[board][chip][5], cell - 8);
	}
}

bool batt_get_balancing_cell_state(int board, int chip, int cell)
{
	if(cell >= 5)
	{
		// We skip S6 in the schematic
		cell += 1;
	}
    if (cell < 8) { // 8 bits per byte in the register
        return GETBIT(m_batt_config[board][chip][4], cell);
    } else {
        return GETBIT(m_batt_config[board][chip][5], cell - 8);
	}
}


HAL_StatusTypeDef batt_config_discharge_timer(DischargeTimerLength length) {
    if (length >= INVALID_DT_TIME) {
        return HAL_ERROR;
    }

    for (int board = 0; board < NUM_BOARDS; board++) {
		for(int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++) {	
			m_batt_config[board][chip][5] |= (length << 4);
		}
    }

    return HAL_OK;
}


#endif
