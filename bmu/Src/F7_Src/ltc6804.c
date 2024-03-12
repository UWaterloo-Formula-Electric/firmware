/* 
 * Owen Brake - May 2021
 * This is the LTC6804-1 chip architecture, the current plan is that it uses 
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
#define ADCV_BROADCAST_BYTE0 (0x03)
#define ADCV_BYTE0 0x03
#define ADCV_BYTE1 0x60

// // Use normal MD (27kHz), Discharge not permission, all channels
// #define ADCV_BROADCAST_BYTE0 (0x02)
// #define ADCV_BYTE0 0x02
// #define ADCV_BYTE1 0xE0

// Use fast MD (27kHz), Discharge not permission, all channels and GPIO 5
#define ADAX_BROADCAST_BYTE0 (0x05)
#define ADAX_BYTE0 0x05
#define ADAX_BYTE1 0x65

#define RDSTATA_BYTE0 0x00
#define RDSTATA_BYTE1 0x10

#define RDSTATB_BYTE0 0x00
#define RDSTATB_BYTE1 0x12

// Use normal MD (7kHz), Discharge not permission, all channels
#define ADOW_BROADCAST_BYTE0 (0x03)
#define ADOW_BYTE0 0x03
#define ADOW_BYTE1(PUP) (0x28 | ((PUP)<<6))

// debugging
#define ADSTAT_BYTE0 0x05
#define ADSTAT_BYTE1 0x69 // check SOC

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


open_wire_failure_t open_wire_failure[NUM_BOARDS * CELLS_PER_BOARD];
static uint8_t thermistor_failure[NUM_BOARDS/2][THERMISTORS_PER_SEGMENT];
static uint8_t m_batt_config[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};

void batt_init_chip_configs()
{
	memset(thermistor_failure, 0, NUM_BOARDS/2*THERMISTORS_PER_SEGMENT);
	memset(open_wire_failure, 0, NUM_BOARDS*CELLS_PER_BOARD*sizeof(open_wire_failure_t));
	for(int board = 0; board < NUM_BOARDS; board++) {
		for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++){
			m_batt_config[board][ltc_chip][0] = REFON(1) | ADC_OPT(0) | SWTRD(1);
		}
	}
}


HAL_StatusTypeDef format_and_send_config(uint8_t config[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE])
{
	const size_t BUFF_SIZE = (COMMAND_SIZE + PEC_SIZE) + ((BATT_CONFIG_SIZE + PEC_SIZE) * NUM_BOARDS);
	uint8_t txBuffer[BUFF_SIZE];
	if (batt_format_write_config_command(WRCFG_BYTE0, WRCFG_BYTE1, txBuffer, config, BATT_CONFIG_SIZE) != HAL_OK) {
		ERROR_PRINT("Failed to send write config command\n");
		return HAL_ERROR;
	}

	// Send command + data
	if (batt_spi_tx(txBuffer, BUFF_SIZE) != HAL_OK)
	{
		ERROR_PRINT("Failed to transmit config to AMS board\n");
		return HAL_ERROR;
	}

	return HAL_OK;
}

/* TODO: GET RID OF THIS FUNCTION */
HAL_StatusTypeDef batt_write_config()
{
	format_and_send_config(m_batt_config);
    return HAL_OK;
}

#define INVALID_DATA 0xFF
static uint32_t PEC_count = 0;
static uint32_t last_PEC_tick = 0;


/*
TODO: Should probably clean this up. Call it smth different or change param names
	first_byte, second_byte: 	11 bit command
	data_buffer: 				full size of the output buffer (num_boards*response_size)
	response_size: 				size of one board's response
*/
static HAL_StatusTypeDef batt_read_data(uint8_t first_byte, uint8_t second_byte, uint8_t* data_buffer, unsigned int response_size){
    const size_t BUFF_SIZE = COMMAND_SIZE + PEC_SIZE + ((response_size + PEC_SIZE) * NUM_BOARDS);
    const size_t DATA_START_IDX = COMMAND_SIZE + PEC_SIZE;
    uint8_t rxBuffer[BUFF_SIZE];
    uint8_t txBuffer[BUFF_SIZE];
	memset(rxBuffer, 0xFF, BUFF_SIZE);
	memset(txBuffer, 0xFF, BUFF_SIZE);
	if (batt_format_command(first_byte, second_byte, txBuffer) != HAL_OK) {
		ERROR_PRINT("Failed to send write config command\n");
		return HAL_ERROR;
	}

	if (spi_tx_rx(txBuffer, rxBuffer, BUFF_SIZE) != HAL_OK) {
		ERROR_PRINT("Failed to send read data command\n");
		return HAL_ERROR;
	}
	
	for (int board = 0; board < NUM_BOARDS; ++board)
	{
		const uint16_t startOfData = DATA_START_IDX + (board * (response_size + PEC_SIZE));
		if (checkPEC(&(rxBuffer[startOfData]), response_size) != HAL_OK)
		{
			DEBUG_PRINT("PEC ERROR on board %d config\r\n", board);
			PEC_count++;
			return HAL_ERROR;
		}
	}


	if(xTaskGetTickCount() - last_PEC_tick > 10000)
	{
		PEC_count = 0;
		last_PEC_tick = xTaskGetTickCount();
	}

	for(int board = 0; board < NUM_BOARDS; board++) {
		memcpy(&(data_buffer[board*response_size]), &(rxBuffer[DATA_START_IDX + (board * (response_size + PEC_SIZE))]), response_size);
	}
	return HAL_OK;
}

HAL_StatusTypeDef batt_read_config(uint8_t config[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE])
{
	const uint8_t response_buffer_size = NUM_BOARDS * BATT_CONFIG_SIZE;
	uint8_t response_buffer[response_buffer_size];
	batt_read_data(RDCFG_BYTE0, RDCFG_BYTE1, response_buffer, BATT_CONFIG_SIZE);

	for(int board = 0; board < NUM_BOARDS; board++){
		memcpy(&(config[board][0][0]), &(response_buffer[board * BATT_CONFIG_SIZE]), BATT_CONFIG_SIZE);
	}

	return HAL_OK;
}

HAL_StatusTypeDef batt_verify_config(){
	uint8_t config_buffer[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};
	if(batt_read_config(config_buffer) != HAL_OK){
		ERROR_PRINT("Failed to read config");
		return HAL_ERROR;
	}

    vTaskDelay(T_REFUP_MS); // Let core state machine transition from STANDBY to REFUP
	
	// Verify was set correctly
	for(int board = 0; board < NUM_BOARDS; board++) {
		for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++) {
			DEBUG_PRINT("\r\nConfig Read A, Board %d, Chip %d: ", board, ltc_chip); 
			for(int buff_byte = 0; buff_byte < BATT_CONFIG_SIZE; buff_byte++) {
				DEBUG_PRINT("0x%x ", config_buffer[board][ltc_chip][buff_byte]);
				if((m_batt_config[board][ltc_chip][buff_byte] & 0x7F) != (config_buffer[board][ltc_chip][buff_byte] & 0x7F)) {
					ERROR_PRINT("\n ERROR: board: %d, ltc_chip: %d, buff_byte %d, %u != %u  \n", board, ltc_chip, buff_byte, m_batt_config[board][ltc_chip][buff_byte], config_buffer[board][ltc_chip][buff_byte]);
					return HAL_ERROR;
				}
			}
			DEBUG_PRINT("\n");
		}
	}
	return HAL_OK;
}

// HAL_StatusTypeDef batt_check_stat_A(void)
// {
// 	uint8_t register_size = 2;
//     uint8_t response_buffer[register_size];
// 	wakeup_sleep();
// 	batt_broadcast_command(ADSTAT);
// 	wakeup_idle();
//     if (batt_read_data(RDSTATA_BYTE0, RDSTATA_BYTE1, response_buffer, register_size) != HAL_OK) {
//         DEBUG_PRINT("Checking chip status failed\n");
//         return HAL_ERROR;
//     }
// 	DEBUG_PRINT("Status Register A: ");
// 	for (int i = 0; i < register_size; i++) {
// 		DEBUG_PRINT("0x%x ", response_buffer[i]);
// 	}
// 	DEBUG_PRINT(" ");
// 	return HAL_OK;
// }

HAL_StatusTypeDef batt_send_command(ltc_command_t curr_command, bool broadcast, size_t board, size_t ltc_chip) {
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
			command_byte_low = ADAX_BROADCAST_BYTE0;
			command_byte_low = ADAX_BROADCAST_BYTE0;
			command_byte_high = ADAX_BYTE1;
			break;
		}
		case(ADOW_UP):
		{
			if(broadcast)
			{
				command_byte_low = ADOW_BROADCAST_BYTE0;
			}
			else
			{
				command_byte_low = ADOW_BYTE0;
			}
			command_byte_high = ADOW_BYTE1(1);
			break;
		}
		case(ADOW_DOWN):
		{
			if(broadcast)
			{
				command_byte_low = ADOW_BROADCAST_BYTE0;
			}
			else
			{
				command_byte_low = ADOW_BYTE0;
			}
			command_byte_high = ADOW_BYTE1(0);
			break;
		}
		case(ADSTAT):
		{
			command_byte_low = ADSTAT_BYTE0;
			command_byte_high = ADSTAT_BYTE1;
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
	if(batt_send_command(curr_command, true, 0, 0) != HAL_OK){
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
	uint8_t cell_indexes_allocated = 0;

	for (int block = 0; block < VOLTAGE_BLOCKS_PER_CHIP; block++) {
		uint8_t cmdByteLow, cmdByteHigh;
		// Select appropriate voltage register group
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

		// Voltage values for one block from one boards
		uint8_t adc_vals[NUM_BOARDS * VOLTAGE_BLOCK_SIZE] = {0};

		if (batt_spi_wakeup(false /* not sleeping*/))
		{
			return HAL_ERROR;
		}
		
		if(batt_read_data(cmdByteLow, cmdByteHigh, adc_vals, VOLTAGE_BLOCK_SIZE) != HAL_OK) {
			DEBUG_PRINT("Failed AMS voltage block read (block %u)\r\n", block);
		}

		const uint8_t block_lowest_cell_num = (block * VOLTAGES_PER_BLOCK) + 1; // +1 as LTC cell numbering is not 0 indexed
		
		for (int block_index = 0; block_index < VOLTAGES_PER_BLOCK; block_index++)
		{
			const uint8_t ltc_cell_num = block_lowest_cell_num + block_index;
			// Only 10 of the 12 voltage sense lines on the chip are are useful
			// pins C6 is connected to CELL5 and C12 is connected to Cell10 which we are the measurements we discard
			if(ltc_cell_num == 6 || ltc_cell_num == 12)
			{
				continue;
			}

			for (int board = 0; board < NUM_BOARDS; ++board)
			{
				const size_t cell_voltage_data_index = (board * VOLTAGE_BLOCK_SIZE) + (block_index * CELL_VOLTAGE_SIZE_BYTES);
				const uint16_t adc_reading = ((uint16_t) (adc_vals[(cell_voltage_data_index + 1)] << 8 | adc_vals[cell_voltage_data_index]));
				
				const size_t bmu_cell_idx = (board * CELLS_PER_BOARD) + cell_indexes_allocated;
				cell_voltage_array[bmu_cell_idx] = ((float)adc_reading) / VOLTAGE_REGISTER_COUNTS_PER_VOLT;

				if(voltage_operation == OPEN_WIRE)
				{
					open_wire_failure[bmu_cell_idx].num_times_consec = 0;
				}
			}
			++cell_indexes_allocated;
		}
	}

    return HAL_OK;
}


void batt_set_temp_config(size_t channel) {
	const uint8_t gpioPins = channel;
	for (int board = 0; board < NUM_BOARDS; board++)
    {
		// Set the external MUX to channel we want to read. MUX pin is selected via GPIO2, GPIO3, GPIO4, LSB first.
		m_batt_config[board][0][0] = (1<<GPIO5_POS) | ((gpioPins & 0xFF) << GPIO1_POS) | REFON(1) | ADC_OPT(0) | SWTRD(1);
		// m_batt_config[board][0][0] &= ~(0xF << GPIO1_POS); // Mask out the last config
		// m_batt_config[board][0][0] |= (1 << GPIO5_POS) | ((channel & 0xF) << GPIO1_POS);
	}
}


HAL_StatusTypeDef batt_read_thermistors(size_t channel, float *cell_temp_array) {
	// adc values for one block from all boards
	uint8_t adc_vals[AUX_BLOCK_SIZE * NUM_BOARDS] = {0};
			
	if(batt_read_data(RDAUXB_BYTE0, RDAUXB_BYTE1, adc_vals, AUX_BLOCK_SIZE) != HAL_OK) {
		DEBUG_PRINT("Failed to read LTC AUX B register\r\n");
		return HAL_ERROR;
	}

	for(int board = 0; board < NUM_BOARDS; board++) {
		// shifting index due to skipping certain thermistors on the AMS boards. An explaination exists above the batt_read_cell_temps function
		if (channel == 6 && board%2 == 1) {
			continue;
		}
		size_t cellIdx = (board*13 + (board+1)/2) + channel;
		if (channel >= 9) {
			if (board%2 == 0) {
				cellIdx = cellIdx - 2;
			} else {
				cellIdx = cellIdx - 3;
			}
		}
		
		// We only use the first GPIO register, 2 bytes out of the total 6 in adc_vals
		uint16_t adcCounts = ((uint16_t) (adc_vals[TEMP_ADC_IDX_HIGH + (board * AUX_BLOCK_SIZE)] << 8
									| adc_vals[TEMP_ADC_IDX_LOW + (board * AUX_BLOCK_SIZE)]));
		float voltageThermistor = ((float)adcCounts) / VOLTAGE_REGISTER_COUNTS_PER_VOLT;
		cell_temp_array[cellIdx] = batt_convert_voltage_to_temp(voltageThermistor);
	}
	return HAL_OK;
}



void batt_set_balancing_cell (int board, int chip, int cell)
{
	if(cell + 1 >= 6) // +1 because LTC cell numbering is not 0 indexed.
	{
		// LTC C6 is not used so bump index up
		cell++;
	}
    if (cell < 8) { // 8 bits per byte in the register
        SETBIT(m_batt_config[board][chip][4], cell);
    } else { // This register byte only contains 4 bits
		SETBIT(m_batt_config[board][chip][5], cell-8);
	}
}


void batt_unset_balancing_cell (int board, int chip, int cell)
{
	if(cell + 1 >= 6) // +1 because LTC cell numbering is not 0 indexed.
	{
		// LTC C6 is not used so bump index up
		cell++;
	}
    if (cell < 8) { // 8 bits per byte in the register
        CLEARBIT(m_batt_config[board][chip][4], cell);
    } else {
        CLEARBIT(m_batt_config[board][chip][5], cell - 8);
	}
}

bool batt_get_balancing_cell_state(int board, int chip, int cell)
{
	if(cell + 1 >= 6) // +1 because LTC cell numbering is not 0 indexed.
	{
		// LTC C6 is not used so bump index up
		cell++;
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
