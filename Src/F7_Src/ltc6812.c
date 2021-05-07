#include "ltc_chip_interface.h"
/**
  *****************************************************************************
  * @file    ltc6812.c
  * @author  Richard Matthews and Owen Brake
  * @brief   Module for communicating with ltc6812 chips on the AMS boards
  * @details The ltc6812 chips are battery monitoring chips, with one on each
  * AMS board connected to 16(subject to change) battery cells. The chips measure the voltage and
  * temperature of the cells. They communicate with the BMU over isoSPI (an
  * electrically isolated SPI interface). This file implements a driver for
  * communicating with the ltc6812 chips
  *
  ******************************************************************************
  */

#if LTC_CHIP == LTC_CHIP_6812

// Command Bytes
#define WRCFGA_BYTE0 (0x00)
#define WRCFGA_BYTE1 0x01

#define WRCFGB_BYTE0 (0x00)
#define WRCFGB_BYTE1 0x24

#define RDCFGA_BYTE0 (0x00)
#define RDCFGA_BYTE1 0x02

#define RDCFGB_BYTE0 (0x00)
#define RDCFGB_BYTE1 0x26

#define RDCVA_BYTE0 (0x00)
#define RDCVA_BYTE1 0x04

#define RDCVB_BYTE0 (0x00)
#define RDCVB_BYTE1 0x06

#define RDCVC_BYTE0 (0x00)
#define RDCVC_BYTE1 0x08

#define RDCVD_BYTE0 (0x00)
#define RDCVD_BYTE1 0x0a

#define RDCVE_BYTE0 (0x00)
#define RDCVE_BYTE1 0x09

#define PLADC_BYTE0  (0x07)
#define PLADC_BYTE1 0x14

#define CLRSTAT_BYTE0 (0x07)
#define CLRSTAT_BYTE1 0x13

#define CLRCELL_BYTE0 (0x07)
#define CLRCELL_BYTE1 0x11

#define CLRAUX_BYTE0 (0x07)
#define CLRAUX_BYTE1 0x12

// For reading auxiliary register group A
#define RDAUXA_BYTE0 (0x00)
#define RDAUXA_BYTE1 0x0c

// For reading auxiliary register group B
#define RDAUXB_BYTE0 (0x00)
#define RDAUXB_BYTE1 0x0e

// Use normal MD (7kHz), Discharge not permission, all channels and GPIO 1 & 2
#define ADCVAX_BYTE0 (0x05)
#define ADCVAX_BYTE1 0x6f

// Use normal MD (7kHz), Discharge not permission, all channels
#define ADCV_BYTE0 (0x03)
#define ADCV_BYTE1 0x60

// Use normal MD (7kHz), Discharge not permission, all channels and GPIO 1 & 2
#define ADAX_BYTE0 (0x05)
#define ADAX_BYTE1 0x65

#define RDSTATB_BYTE0 (0x00)
#define RDSTATB_BYTE1 0x12

#define ADOW_BYTE0 (0x03)
#define ADOW_BYTE1(PUP) (0x28 | ((PUP)<<6))

#define BATT_CONFIG_SIZE 6    // Size of Config per Register
#define COMMAND_SIZE 2
#define PEC_SIZE 2
#define VOLTAGE_BLOCK_SIZE 6
#define AUX_BLOCK_SIZE 6
#define CELL_VOLTAGE_SIZE_BYTES 2

#define VOLTAGE_BLOCKS_PER_CHIP    5   // Number of voltage blocks per AMS board
#define VOLTAGES_PER_BLOCK          3   // Number of voltage reading per block


static uint8_t m_batt_config_a[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};
static uint8_t m_batt_config_b[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};


void batt_init_chip_configs()
{
    for(int board = 0; board < NUM_BOARDS; board++) {
		for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++) {
			m_batt_config_a[board][ltc_chip][0] = REFON(1) |          // Turn on reference
									ADC_OPT(0);         // We use fast ADC speed so this has to be 0
			m_batt_config_a[board][ltc_chip][4] = 0x00;                 // Disable the discharge bytes for now
			m_batt_config_a[board][ltc_chip][5] = 0x00;
			m_batt_config_b[board][ltc_chip][0] = 0x00; // Clear GPIO, DCC and RSVD
		}
	}
}

static HAL_StatusTypeDef format_and_send_config(
		uint8_t first_config_byte,
		uint8_t second_config_byte,
		uint8_t config_buffer[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE])
{
	
	const size_t BUFF_SIZE = COMMAND_SIZE + PEC_SIZE + ((BATT_CONFIG_SIZE + PEC_SIZE) * NUM_BOARDS * NUM_LTC_CHIPS_PER_BOARD);
	uint8_t txBuffer[BUFF_SIZE];

	if (batt_format_command(first_config_byte, second_config_byte, txBuffer) != HAL_OK) {
		ERROR_PRINT("Failed to send write config command\n");
		return HAL_ERROR;
	}

	// Fill data, note that config register is sent in reverse order, as the
	// daisy chain acts as a shift register
	// Therefore, we send data destined for the last board first
	for (int board = (NUM_BOARDS-1); board >= 0; board--) {
		for(int ltc_chip = (NUM_LTC_CHIPS_PER_BOARD-1); ltc_chip >= 0; ltc_chip--){
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

	return 0;
}

/**
 * @brief: Read data from the daisy chain
 *
 * @param cmdByteLow: cmd to send
 * @param cmdByteHigh: cmd to send
 * @param dataOutBuffer: buffer to put read data into (PECs removed after
 * verification)
 * @param readSizePerBoard: amount of data to read per board
 *
 * @return HAL_StatusTypeDef
 */
static HAL_StatusTypeDef batt_read_data(uint8_t cmdByteLow, uint8_t cmdByteHigh, uint8_t *dataOutBuffer, size_t readSizePerChip)
{
    const size_t BUFF_SIZE = COMMAND_SIZE + PEC_SIZE + NUM_BOARDS * (readSizePerChip + PEC_SIZE) * NUM_LTC_CHIPS_PER_BOARD;
    const size_t DATA_START_IDX = COMMAND_SIZE + PEC_SIZE;
    uint8_t rxBuffer[BUFF_SIZE];
    uint8_t txBuffer[BUFF_SIZE];

    if (batt_format_command(cmdByteLow, cmdByteHigh, txBuffer) != HAL_OK) {
        return HAL_ERROR;
    }

    fillDummyBytes(&(txBuffer[DATA_START_IDX]), NUM_BOARDS * NUM_LTC_CHIPS_PER_BOARD * (readSizePerChip + PEC_SIZE));

    if (spi_tx_rx(txBuffer, rxBuffer, BUFF_SIZE) != HAL_OK) {
        ERROR_PRINT("Failed to send read data command\n");
        return HAL_ERROR;
    }

    for (int board = 0; board < NUM_BOARDS; board++) {
    	for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++){
			int dataStartIdx = DATA_START_IDX + (PEC_SIZE+readSizePerChip) * (board * NUM_LTC_CHIPS_PER_BOARD + ltc_chip);
			if (checkPEC(&(rxBuffer[dataStartIdx]), readSizePerChip) != HAL_OK)
			{
				ERROR_PRINT("PEC mismatch\n");
				return HAL_ERROR;
			}

			// Copy data out
			for (int i = 0; i < readSizePerChip; i++) {
				dataOutBuffer[(board * NUM_LTC_CHIPS_PER_BOARD + ltc_chip)*readSizePerChip + i] = rxBuffer[dataStartIdx + i];
			}
		}
    }

    return HAL_OK;
}


HAL_StatusTypeDef batt_write_config()
{
	// Send config_a
	HAL_StatusTypeDef config_a_retval = format_and_send_config(WRCFGA_BYTE0, WRCFGA_BYTE1, m_batt_config_a);
	
	if(0 != config_a_retval)
	{
		return config_a_retval;
	}
	
	// Send config_b
	HAL_StatusTypeDef config_b_retval = format_and_send_config(WRCFGB_BYTE0, WRCFGB_BYTE1, m_batt_config_b);
	return config_b_retval;
}


static HAL_StatusTypeDef batt_read_config(uint8_t config_a[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE], uint8_t config_b[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE])
{
    if (batt_read_data(RDCFGA_BYTE0, RDCFGA_BYTE1,(uint8_t*) config_a, BATT_CONFIG_SIZE) != HAL_OK)
    {
        ERROR_PRINT("Failed to read config A\n");
        return HAL_ERROR;
    }
    if(batt_read_data(RDCFGB_BYTE0, RDCFGB_BYTE1,(uint8_t*) config_b, BATT_CONFIG_SIZE) != HAL_OK){
		ERROR_PRINT("Failed to read config B\n");
		return HAL_ERROR;
	}

    return HAL_OK;
}

HAL_StatusTypeDef batt_verify_config(){
uint8_t config_buffer_a[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};
uint8_t config_buffer_b[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};
if(batt_read_config(config_buffer_a, config_buffer_b) != HAL_OK){
	ERROR_PRINT("Failed to read config");
	return HAL_ERROR;
}

vTaskDelay(T_REFUP_MS);

// Verify was set correctly
for(int board = 0; board < NUM_BOARDS; board++) {
	for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++) {
		for(int buff_byte = 0; buff_byte < BATT_CONFIG_SIZE; buff_byte++) {
			DEBUG_PRINT("Read Config A, board %d, ltc_chip %d, byte_val: %d", board, ltc_chip, config_buffer_a[board][ltc_chip][buff_byte]);
			DEBUG_PRINT("Read Config B, board %d, ltc_chip %d, byte_val: %d", board, ltc_chip, config_buffer_b[board][ltc_chip][buff_byte]);
			if(m_batt_config_a[board][ltc_chip][buff_byte] != config_buffer_a[board][ltc_chip][buff_byte]) {
				ERROR_PRINT("ERROR: m_batt_config_a board: %d, ltc_chip: %d, buff_byte %d, mismatch", board, ltc_chip, buff_byte);
				return HAL_ERROR;
			}
			if(m_batt_config_b[board][ltc_chip][buff_byte] != config_buffer_b[board][ltc_chip][buff_byte]) {
				ERROR_PRINT("ERROR: m_batt_config_b board: %d, ltc_chip: %d, buff_byte %d, mismatch", board, ltc_chip, buff_byte);
				return HAL_ERROR;
			}
		}
	}
}
return HAL_OK;
}

HAL_StatusTypeDef batt_broadcast_command(ltc_command_t curr_command) {
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




/**
* @brief Read back cell voltages.
*
* This assumes that the command to initiate ADC readings has been sent already
* and the appropriate amount of time has elapsed for readings to finish.
* Used for both @ref batt_read_cell_voltages and
* @ref performOpenCircuitTestReading.
*
* @param[out] cell_voltage_array: Pointer to buffer to store cell voltages
*
* @return HAL_StatusTypeDef
*/
HAL_StatusTypeDef batt_readBackCellVoltage(float *cell_voltage_array)
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
	if(block == 0)
	{
		cmdByteLow = RDCVA_BYTE0;
		cmdByteHigh = RDCVA_BYTE1;
	}
	else if(block == 1)
	{
		cmdByteLow = RDCVB_BYTE0;
		cmdByteHigh = RDCVB_BYTE1;
	}
	else if(block == 2)
	{
		cmdByteLow = RDCVC_BYTE0;
		cmdByteHigh = RDCVC_BYTE1;
	}
	else if(block == 3)
	{
		cmdByteLow = RDCVD_BYTE0;
		cmdByteHigh = RDCVD_BYTE1;
	} else if(block == 4) {
		cmdByteLow = RDCVE_BYTE0;
		cmdByteHigh = RDCVE_BYTE1;
	} else {
		ERROR_PRINT("Attempt to access unkown voltage block\n");
		return HAL_ERROR;
	}

	if (batt_read_data(cmdByteLow, cmdByteHigh, adc_vals, VOLTAGE_BLOCK_SIZE)
			!= HAL_OK) {
		ERROR_PRINT("Failed to read voltage block %d\n", block);
		return HAL_ERROR;
	}

	// Have to read 3 battery voltages from each cell voltage register
	for (int board = 0; board < NUM_BOARDS; board++) {
		for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++){
			for (int cvreg = 0; cvreg < VOLTAGES_PER_BLOCK; cvreg ++)
			{
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
    	for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++) 
    	{
			// Set the external MUX to channel we want to read. MUX pin is selected via GPIO2, GPIO3, GPIO4, LSB first.
			uint16_t gpioPins = channel;
			m_batt_config_a[board][ltc_chip][0] = (1<<GPIO5_POS) | ((gpioPins & 0xFF) << GPIO1_POS) | REFON(1) | ADC_OPT(0);

			// We have this mask so we preserve the DCC config in the first 4 bits
			m_batt_config_b[board][ltc_chip][0] = (m_batt_config_b[board][ltc_chip][0] & 0xFF00) | (gpioPins >> BITS_PER_BYTE);
			
		}
	}
}


void batt_set_balancing_cell (int board, int chip, int cell) {
    if (cell < 8) { // 8 bits per byte in the register
        SETBIT(m_batt_config_a[board][chip][4], cell);
    } else if (cell < 12) { // This register byte only contains 4 bits
		SETBIT(m_batt_config_a[board][chip][5], cell - 8);
	} else {
		// cell - 12 + 4
		// The 12 is because the first DCC bit on m_batt_config_b is cell 12(0 indexed)
		// The 4 is because DCC13(1 indexed) is on the (1<<4) bit 
        SETBIT(m_batt_config_b[board][chip][0], cell - 12 + 4);
    }
}


void batt_unset_balancing_cell (int board, int chip, int cell)
{
    if (cell < 8) { // 8 bits per byte in the register
        CLEARBIT(m_batt_config_a[board][chip][4], cell);
    } else if (cell < 12){
        CLEARBIT(m_batt_config_a[board][chip][5], cell - 8);
    } else {
		CLEARBIT(m_batt_config_b[board][chip][0], cell - 12 + 4);
	}
}


bool batt_get_balancing_cell_state(int board, int chip, int cell)
{
    if (cell < 8) { // 8 bits per byte in the register
        return GETBIT(m_batt_config_a[board][chip][4], cell);
    } else if (cell < 12) {
        return GETBIT(m_batt_config_a[board][chip][5], cell - 8);
    } else {
		return GETBIT(m_batt_config_b[board][chip][0], cell - 12 + 4);
	}
}
HAL_StatusTypeDef batt_read_thermistors(size_t channel, float *cell_temp_array) {

    // adc values for one block from all boards
    uint8_t adc_vals[AUX_BLOCK_SIZE * NUM_BOARDS * NUM_LTC_CHIPS_PER_BOARD] = {0};

    if (batt_read_data(RDAUXB_BYTE0, RDAUXB_BYTE1, adc_vals, AUX_BLOCK_SIZE) != HAL_OK)
    {
        ERROR_PRINT("Failed to read temp adc vals\n");
        return HAL_ERROR;
    }

    for (int board = 0; board < NUM_BOARDS; board++) {
    	for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++) {
			size_t cellIdx = board * CELLS_PER_BOARD + ltc_chip * CELLS_PER_CHIP + channel;
			// We only use one GPIO input to measure temperatures, so pick that out
			size_t boardStartIdx = (board * NUM_LTC_CHIPS_PER_BOARD + ltc_chip) * (AUX_BLOCK_SIZE);
			uint16_t temp = ((uint16_t) (adc_vals[boardStartIdx + TEMP_ADC_IDX_HIGH] << 8
										| adc_vals[boardStartIdx + TEMP_ADC_IDX_LOW]));
			float voltageThermistor = ((float)temp) / VOLTAGE_REGISTER_COUNTS_PER_VOLT;
			cell_temp_array[cellIdx] = batt_convert_voltage_to_temp(voltageThermistor);
		}
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_config_discharge_timer(DischargeTimerLength length) {
    if (length >= INVALID_DT_TIME) {
        return HAL_ERROR;
    }

    for (int board = 0; board < NUM_BOARDS; board++) {
		for(int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++) {	
			m_batt_config_a[board][chip][5] |= (length << 4);
		}
    }

    return HAL_OK;
}


#endif
