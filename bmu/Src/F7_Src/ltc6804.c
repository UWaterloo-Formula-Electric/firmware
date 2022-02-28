/* 
 * Owen Brake - May 2021
 * This is the LTC6804 chip architecture, the current plan is that it uses 
 *
 * */
#include "ltc_chip_interface.h"

#if LTC_CHIP == LTC_CHIP_6804

#define WRCFG_BYTE0(ADDRESS) ((0x80) | (ADDRESS << 3))
#define WRCFG_BYTE1 0x01

#define RDCFG_BYTE0(ADDRESS) ((0x80) | (ADDRESS << 3))
#define RDCFG_BYTE1 0x02

#define RDCVA_BYTE0(ADDRESS) ((0x80) | (ADDRESS << 3))
#define RDCVA_BYTE1 0x04

#define RDCVB_BYTE0(ADDRESS) ((0x80) | (ADDRESS << 3))
#define RDCVB_BYTE1 0x06

#define RDCVC_BYTE0(ADDRESS) ((0x80) | (ADDRESS << 3))
#define RDCVC_BYTE1 0x08

#define RDCVD_BYTE0(ADDRESS) ((0x80) | (ADDRESS << 3))
#define RDCVD_BYTE1 0x0a

#define PLADC_BYTE0(ADDRESS) ((0x87) | (ADDRESS << 3))
#define PLADC_BYTE1 0x14

#define CLRSTAT_BYTE0(ADDRESS) ((0x87) | (ADDRESS << 3))
#define CLRSTAT_BYTE1 0x13

#define CLRCELL_BYTE0(ADDRESS) ((0x87) | (ADDRESS << 3))
#define CLRCELL_BYTE1 0x11

#define CLRAUX_BYTE0(ADDRESS) ((0x87) | (ADDRESS << 3))
#define CLRAUX_BYTE1 0x12

// For reading auxiliary register group A
#define RDAUXA_BYTE0(ADDRESS) ((0x80) | (ADDRESS << 3))
#define RDAUXA_BYTE1 0x0c

// For reading auxiliary register group B
#define RDAUXB_BYTE0(ADDRESS) ((0x80) | (ADDRESS << 3))
#define RDAUXB_BYTE1 0x0e

// Use normal MD (7kHz), Discharge not permission, all channels and GPIO 1 & 2
#define ADCVAX_BYTE0(ADDRESS) ((0x85) | (ADDRESS << 3))
#define ADCVAX_BYTE1 0x6f

// Use normal MD (7kHz), Discharge not permission, all channels
#define ADCV_BYTE0(ADDRESS) ((0x83) | (ADDRESS << 3))
#define ADCV_BYTE1 0x60

// Use fast MD (27kHz), Discharge not permission, all channels and GPIO 1 & 2
#define ADAX_BYTE0(ADDRESS) ((0x85) | (ADDRESS << 3))
#define ADAX_BYTE1 0x65

#define RDSTATB_BYTE0(ADDRESS) ((0x80) | (ADDRESS << 3))
#define RDSTATB_BYTE1 0x12

#define ADOW_BYTE0(ADDRESS) ((0x83) | (ADDRESS << 3))
#define ADOW_BYTE1(PUP) (0x28 | ((PUP)<<6))


#define ADC_OPT(en) (en << 0) // Since we're using the normal 7kHz mode
#define SWTRD(en) (en << 1) // We're not using the software time
#define REFON(en) (en << 2)

#define BATT_CONFIG_SIZE 6    // Size of Config per Register
#define COMMAND_SIZE 2
#define PEC_SIZE 2
#define VOLTAGE_BLOCK_SIZE 6
#define AUX_BLOCK_SIZE 6
#define CELL_VOLTAGE_SIZE_BYTES 2

#define VOLTAGE_BLOCKS_PER_CHIP    4   // Number of voltage blocks per AMS board
#define VOLTAGES_PER_BLOCK          3   // Number of voltage reading per block

#define THERMISTOR_CHIP 0


static const uint8_t LTC_ADDRESS[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD] = {
	{0, 1},
	{2, 3},
	{4, 5},
	{6, 7},
	{8, 9}
};


static uint8_t m_batt_config[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};

void batt_init_chip_configs()
{
	for(int board = 0; board < NUM_BOARDS; board++) {
		for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++){
			m_batt_config[board][ltc_chip][0] = REFON(1) | ADC_OPT(0) | SWTRD(1);
		}
	}
}


HAL_StatusTypeDef format_and_send_config(
		uint8_t address,
		uint8_t config_buffer[BATT_CONFIG_SIZE])
{
	
	const size_t BUFF_SIZE = COMMAND_SIZE + PEC_SIZE + (BATT_CONFIG_SIZE + PEC_SIZE);
	const size_t START_OF_DATA_IDX = COMMAND_SIZE + PEC_SIZE;
	uint8_t txBuffer[BUFF_SIZE];

	if (batt_format_command(WRCFG_BYTE0(address), WRCFG_BYTE1, txBuffer) != HAL_OK) {
		ERROR_PRINT("Failed to send write config command\n");
		return HAL_ERROR;
	}

	for (int dbyte = 0; dbyte < BATT_CONFIG_SIZE; dbyte++)
	{
		txBuffer[START_OF_DATA_IDX + dbyte]
			= config_buffer[dbyte];
	}

	// Data pec
	batt_gen_pec(config_buffer, BATT_CONFIG_SIZE,
				&(txBuffer[START_OF_DATA_IDX + BATT_CONFIG_SIZE]));

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
	// Each chip is individually addressable,
	for(int board = 0; board < NUM_BOARDS; board++){
		for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++){
			format_and_send_config(LTC_ADDRESS[board][ltc_chip], m_batt_config[board][ltc_chip]);
		}
	}
    return HAL_OK;
}

static HAL_StatusTypeDef batt_read_data(uint8_t first_byte, uint8_t second_byte, uint8_t* data_buffer, unsigned int response_size){
    const size_t BUFF_SIZE = COMMAND_SIZE + PEC_SIZE + (response_size + PEC_SIZE);
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
	

	if (checkPEC(&(rxBuffer[DATA_START_IDX]), response_size) != HAL_OK)
	{
		ERROR_PRINT("PEC mismath \n");
		return HAL_ERROR;
	}

	for(int response_byte_i = 0; response_byte_i < response_size; response_byte_i++) {
		data_buffer[response_byte_i] = rxBuffer[DATA_START_IDX + response_byte_i];
	}
	return HAL_OK;
}

HAL_StatusTypeDef batt_read_config(uint8_t config[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE])
{

	for(int board = 0; board < NUM_BOARDS; board++){
		for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++){

			int address = LTC_ADDRESS[board][ltc_chip];
		    uint8_t response_buffer[BATT_CONFIG_SIZE] = {0};

			batt_read_data(RDCFG_BYTE0(address), RDCFG_BYTE1, response_buffer, BATT_CONFIG_SIZE);
			
			for(int i = 0;i < BATT_CONFIG_SIZE; i++){
				config[board][ltc_chip][i] = response_buffer[i];
			}
		}
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
			DEBUG_PRINT("Config Read A, Board %d, Chip %d: ", board, ltc_chip); 
			for(int buff_byte = 0; buff_byte < BATT_CONFIG_SIZE; buff_byte++) {
				DEBUG_PRINT("0x%x", config_buffer[board][ltc_chip][buff_byte]);
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


HAL_StatusTypeDef batt_send_command(ltc_command_t curr_command, size_t board, size_t ltc_chip) {
    const size_t TX_BUFF_SIZE = COMMAND_SIZE + PEC_SIZE;
    uint8_t txBuffer[TX_BUFF_SIZE];
	
	uint8_t command_byte_low, command_byte_high;
	uint8_t address = LTC_ADDRESS[board][ltc_chip];
	switch(curr_command) {
		case(ADCV): 
		{
			command_byte_low = ADCV_BYTE0(address);
			command_byte_high = ADCV_BYTE1;
			break;
		}
		case(ADAX):
		{
			command_byte_low = ADAX_BYTE0(address);
			command_byte_high = ADAX_BYTE1;
			break;
		}
		case(ADOW_UP):
		{
			command_byte_low = ADOW_BYTE0(address);
			command_byte_high = ADOW_BYTE1(1);
			break;
		}
		case(ADOW_DOWN):
		{
			command_byte_low = ADOW_BYTE0(address);
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
	if(batt_send_command(curr_command, 0, 0) != HAL_OK){
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
HAL_StatusTypeDef batt_readBackCellVoltage(float *cell_voltage_array)
{

	if (batt_spi_wakeup(false /* not sleeping*/))
	{
		return HAL_ERROR;
	}
	for (int board = 0; board < NUM_BOARDS; board++){
		for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++) {
			size_t local_cell_idx = 0;
			for (int block = 0; block < VOLTAGE_BLOCKS_PER_CHIP; block++) {
				
				uint8_t address = LTC_ADDRESS[board][ltc_chip]; 
				uint8_t cmdByteLow, cmdByteHigh;
				// Select appropriate voltage register group
				switch(block){
					case 0:
					{
						cmdByteLow = RDCVA_BYTE0(address);
						cmdByteHigh = RDCVA_BYTE1;
						break;
					}
					case 1:
					{
						cmdByteLow = RDCVB_BYTE0(address);
						cmdByteHigh = RDCVB_BYTE1;
						break;
					}
					case 2:
					{
						cmdByteLow = RDCVC_BYTE0(address);
						cmdByteHigh = RDCVC_BYTE1;
						break;
					}
					case 3:
					{
						cmdByteLow = RDCVD_BYTE0(address);
						cmdByteHigh = RDCVD_BYTE1;
						break;
					}
					default:
					{
						ERROR_PRINT("ERROR: Unknown voltage block accessed");
						return HAL_ERROR;
					}
				}
				
				// Voltage values for one block from one boards
				uint8_t adc_vals[VOLTAGE_BLOCK_SIZE] = {0};
				if(batt_read_data(cmdByteLow, cmdByteHigh, adc_vals, VOLTAGE_BLOCK_SIZE) != HAL_OK) {
					DEBUG_PRINT("Failed on board: %d, chip %d, block %d", board, ltc_chip, block);
					ERROR_PRINT("ERROR: Issue reading voltage cell values\n");
					return HAL_ERROR;
				}
				for (int cvreg = 0; cvreg < VOLTAGES_PER_BLOCK; cvreg++)
				{
					if(cvreg + block * VOLTAGES_PER_BLOCK == 4 || cvreg + block * VOLTAGES_PER_BLOCK == 5)
					{
						continue;
					}

					size_t registerIndex = cvreg * CELL_VOLTAGE_SIZE_BYTES;
					size_t cellIdx = (board * NUM_LTC_CHIPS_PER_BOARD + ltc_chip) * CELLS_PER_CHIP + local_cell_idx;

					uint16_t temp = ((uint16_t) (adc_vals[(registerIndex + 1)] << 8 | adc_vals[registerIndex]));
					cell_voltage_array[cellIdx] = ((float)temp) / VOLTAGE_REGISTER_COUNTS_PER_VOLT;
					local_cell_idx++;
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
		
		uint8_t address = LTC_ADDRESS[board][THERMISTOR_CHIP];
		
		if(batt_read_data(RDAUXB_BYTE0(address), RDAUXB_BYTE1, adc_vals, AUX_BLOCK_SIZE) != HAL_OK) {
			ERROR_PRINT("ERROR: Error reading thermistor values over SPI");
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
    if (cell < 8) { // 8 bits per byte in the register
        SETBIT(m_batt_config[board][chip][4], cell);
    } else { // This register byte only contains 4 bits
		SETBIT(m_batt_config[board][chip][5], cell - 8);
	}
}


void batt_unset_balancing_cell (int board, int chip, int cell)
{
    if (cell < 8) { // 8 bits per byte in the register
        CLEARBIT(m_batt_config[board][chip][4], cell);
    } else {
        CLEARBIT(m_batt_config[board][chip][5], cell - 8);
	}
}

bool batt_get_balancing_cell_state(int board, int chip, int cell)
{
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
