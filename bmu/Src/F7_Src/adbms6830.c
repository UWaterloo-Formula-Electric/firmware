/**
 * @file    adbms6830.c
 * @author  Mohil Kadakia
 * @brief   Module for communicating with ADBMS6830B chips on the AMS boards
 * @details Datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/adbms6830b.pdf
 *
 */

#include "ltc_chip_interface.h"
#include "string.h"

#if LTC_CHIP == ADBMS_CHIP_6830B

// Table 50 Configuration Register Group A
#define WRCFGA_BYTE0  0x00
#define WRCFGA_BYTE1  0x01

#define WRCFGB_BYTE0  0x00
#define WRCFGB_BYTE1  0x24

#define RDCFGA_BYTE0  0x00
#define RDCFGA_BYTE1  0x02

#define RDCFGB_BYTE0  0x00
#define RDCFGB_BYTE1  0x26

#define RDCVA_BYTE0  0x00
#define RDCVA_BYTE1  0x04

#define RDCVB_BYTE0  0x00
#define RDCVB_BYTE1  0x06

#define RDCVC_BYTE0  0x00
#define RDCVC_BYTE1  0x08

#define RDCVD_BYTE0  0x00
#define RDCVD_BYTE1  0x0A

#define RDCVE_BYTE0  0x00
#define RDCVE_BYTE1  0x09

#define RDCVF_BYTE0  0x00
#define RDCVF_BYTE1  0x0B

#define RDCVALL_BYTE0  0x00
#define RDCVALL_BYTE1  0x0C

#define RDACA_BYTE0  0x00
#define RDACA_BYTE1  0x44

#define RDACB_BYTE0  0x00
#define RDACB_BYTE1  0x46

#define RDACC_BYTE0  0x00
#define RDACC_BYTE1  0x48

#define RDACD_BYTE0  0x00
#define RDACD_BYTE1  0x4A

#define RDACE_BYTE0  0x00
#define RDACE_BYTE1  0x49

#define RDACF_BYTE0  0x00
#define RDACF_BYTE1  0x4B

#define RDACALL_BYTE0  0x00
#define RDACALL_BYTE1  0x4C

#define RDSVA_BYTE0  0x00
#define RDSVA_BYTE1  0x03

#define RDSVB_BYTE0  0x00
#define RDSVB_BYTE1  0x05

#define RDSVC_BYTE0  0x00
#define RDSVC_BYTE1  0x07

#define RDSVD_BYTE0  0x00
#define RDSVD_BYTE1  0x0D

#define RDSVE_BYTE0  0x00
#define RDSVE_BYTE1  0x0E

#define RDSVF_BYTE0  0x00
#define RDSVF_BYTE1  0x0F

#define RDSALL_BYTE0  0x00
#define RDSALL_BYTE1  0x10

#define RDCSALL_BYTE0  0x00
#define RDCSALL_BYTE1  0x11

#define RDACSALL_BYTE0  0x00
#define RDACSALL_BYTE1  0x51

#define RDFCA_BYTE0  0x00
#define RDFCA_BYTE1  0x12

#define RDFCB_BYTE0  0x00
#define RDFCB_BYTE1  0x13

#define RDFCC_BYTE0  0x00
#define RDFCC_BYTE1  0x14

#define RDFCD_BYTE0  0x00
#define RDFCD_BYTE1  0x15

#define RDFCE_BYTE0  0x00
#define RDFCE_BYTE1  0x16

#define RDFCF_BYTE0  0x00
#define RDFCF_BYTE1  0x17

#define RDFCALL_BYTE0  0x00
#define RDFCALL_BYTE1  0x18

#define RDAUXA_BYTE0  0x00
#define RDAUXA_BYTE1  0x19

#define RDAUXB_BYTE0  0x00
#define RDAUXB_BYTE1  0x1A

#define RDAUXC_BYTE0  0x00
#define RDAUXC_BYTE1  0x1B

#define RDAUXD_BYTE0  0x00
#define RDAUXD_BYTE1  0x1F

#define RDRAXA_BYTE0  0x00
#define RDRAXA_BYTE1  0x1C

#define RDRAXB_BYTE0  0x00
#define RDRAXB_BYTE1  0x1D

#define RDRAXC_BYTE0  0x00
#define RDRAXC_BYTE1  0x1E

#define RDRAXD_BYTE0  0x00
#define RDRAXD_BYTE1  0x25

#define RDSTATA_BYTE0  0x00
#define RDSTATA_BYTE1  0x30

#define RDSTATB_BYTE0  0x00
#define RDSTATB_BYTE1  0x31

#define RDSTATC_BYTE0  0x00
#define RDSTATC_BYTE1  0x32

#define RDSTATC_ERR_BYTE0  0x00
#define RDSTATC_ERR_BYTE1  0x72

#define RDSTATD_BYTE0  0x00
#define RDSTATD_BYTE1  0x33

#define RDSTATE_BYTE0  0x00
#define RDSTATE_BYTE1  0x34

#define RDASALL_BYTE0  0x00
#define RDASALL_BYTE1  0x35

#define WRPWMA_BYTE0  0x00
#define WRPWMA_BYTE1  0x20

#define RDPWMA_BYTE0  0x00
#define RDPWMA_BYTE1  0x22

#define WRPWMB_BYTE0  0x00
#define WRPWMB_BYTE1  0x21

#define RDPWMB_BYTE0  0x00
#define RDPWMB_BYTE1  0x23

#define CMDIS_BYTE0  0x00
#define CMDIS_BYTE1  0x40

#define CMEN_BYTE0  0x00
#define CMEN_BYTE1  0x41

#define CMHB2_BYTE0  0x00
#define CMHB2_BYTE1  0x43

#define WRCMCFG_BYTE0  0x00
#define WRCMCFG_BYTE1  0x58

#define RDCMCFG_BYTE0  0x00
#define RDCMCFG_BYTE1  0x59

#define WRCMCELLT_BYTE0  0x00
#define WRCMCELLT_BYTE1  0x5A

#define RDCMCELLT_BYTE0  0x00
#define RDCMCELLT_BYTE1  0x5B

#define WRCMGPIOT_BYTE0  0x00
#define WRCMGPIOT_BYTE1  0x5C

#define RDCMGPIOT_BYTE0  0x00
#define RDCMGPIOT_BYTE1  0x5D

#define CLRCMFLAG_BYTE0  0x00
#define CLRCMFLAG_BYTE1  0x5E

#define RDCMFLAG_BYTE0  0x00
#define RDCMFLAG_BYTE1  0x5F

#define CLRCELL_BYTE0  0x07
#define CLRCELL_BYTE1  0x11

#define CLRFC_BYTE0  0x07
#define CLRFC_BYTE1  0x14

#define CLRAUX_BYTE0  0x07
#define CLRAUX_BYTE1  0x12

#define CLRSPIN_BYTE0  0x07
#define CLRSPIN_BYTE1  0x16

#define CLRFLAG_BYTE0  0x07
#define CLRFLAG_BYTE1  0x17

#define CLOVUV_BYTE0  0x07
#define CLOVUV_BYTE1  0x15

#define PLADC_BYTE0  0x07
#define PLADC_BYTE1  0x18

#define PLCADC_BYTE0  0x07
#define PLCADC_BYTE1  0x1C

#define PLSADC_BYTE0  0x07
#define PLSADC_BYTE1  0x1D

#define PLAUX_BYTE0  0x07
#define PLAUX_BYTE1  0x1E

#define PLAUX2_BYTE0  0x07
#define PLAUX2_BYTE1  0x1F

#define WRCOMM_BYTE0  0x07
#define WRCOMM_BYTE1  0x21

#define RDCOMM_BYTE0  0x07
#define RDCOMM_BYTE1  0x22

#define STCOMM_BYTE0  0x07
#define STCOMM_BYTE1  0x23

#define MUTE_BYTE0  0x00
#define MUTE_BYTE1  0x28

#define UNMUTE_BYTE0  0x00
#define UNMUTE_BYTE1  0x29

#define RDSID_BYTE0  0x00
#define RDSID_BYTE1  0x2C

#define RSTCC_BYTE0  0x00
#define RSTCC_BYTE1  0x2E

#define SNAP_BYTE0  0x00
#define SNAP_BYTE1  0x2D

#define UNSNAP_BYTE0  0x00
#define UNSNAP_BYTE1  0x2F

#define SRST_BYTE0  0x00
#define SRST_BYTE1  0x27

#define ULRR_BYTE0  0x00
#define ULRR_BYTE1  0x38

#define WRRR_BYTE0  0x00
#define WRRR_BYTE1  0x39

#define RDRR_BYTE0  0x00
#define RDRR_BYTE1  0x3A

// ADCV, ADSV, ADAX, ADAX2

// Use normal MD (7kHz), Discharge not permission, all channels
// Might have to change these values later based on desired configuration
#define ADCV_BYTE0 0x02
#define ADCV_BYTE1 0x63

#define ADSV_BYTE0 0x01
#define ADSV_BYTE1 0x6B

// Read from GPIO 5 (MUX output)
#define ADAX_BYTE0 0x05
#define ADAX_BYTE1(PUP) (0x15 | ((PUP)<<7))

#define ADAX2_BYTE0 0x04
#define ADAX2_BYTE1 0x73

// Table 55 Configuration Register Group A
#ifdef REFON
#undef REFON // Remove previous definition
#endif

#define REFON(en)      ((en) << 7)
#define COMM_BK(en)    ((en) << 3)
#define MUTE_ST(en)    ((en) << 4)
#define CTH(en)    ((en) << 0)

// Table 56 Configuration Register Group B
#define DTMEN(en)    ((en) << 7)
#define DTRNG(en)    ((en) << 6)


open_wire_failure_t open_wire_failure[NUM_BOARDS * CELLS_PER_BOARD];
static uint8_t thermistor_failure[NUM_BOARDS/2][THERMISTORS_PER_SEGMENT];
static uint8_t m_batt_configA[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};
static uint8_t m_batt_configB[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};

void batt_init_chip_configs() {
    memset(thermistor_failure, 0, NUM_BOARDS/2*THERMISTORS_PER_SEGMENT*sizeof(uint8_t));
	memset(open_wire_failure, 0, NUM_BOARDS*CELLS_PER_BOARD*sizeof(open_wire_failure_t));

	for(int board = 0; board < NUM_BOARDS; board++) {
		for(int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++){
            // Table 102 Configuration Register A Bit
			// Configuration Register A
            m_batt_configA[board][chip][0] = (REFON(1)) | (CTH(6));
            m_batt_configA[board][chip][5] = (COMM_BK(0)) | (MUTE_ST(0));
            
            // Table 103 Configuration Register B Bit
            // Configuration Register B (UV/OV thresholds)
            // m_batt_configB[board][chip][0] = 0x00;  // VUV LSBs
            // m_batt_configB[board][chip][1] = 0x08;  // VUV[11:8], VOV[3:0]
            // m_batt_configB[board][chip][2] = 0x07;  // VOV[11:4]
		}
	}
}

HAL_StatusTypeDef format_and_send_config(uint8_t configA[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE], uint8_t configB[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE])
{
	const size_t BUFF_SIZE = (COMMAND_SIZE + PEC_SIZE) + ((BATT_CONFIG_SIZE + PEC_SIZE) * NUM_BOARDS);
	uint8_t txBuffer[BUFF_SIZE];
	if (batt_format_write_config_command(WRCFGA_BYTE0, WRCFGA_BYTE1, txBuffer, configA, BATT_CONFIG_SIZE) != HAL_OK) {
		ERROR_PRINT("Failed to send write configA command\n");
		return HAL_ERROR;
	}
	if (batt_format_write_config_command(WRCFGB_BYTE0, WRCFGB_BYTE1, txBuffer, configB, BATT_CONFIG_SIZE) != HAL_OK) {
		ERROR_PRINT("Failed to send write configB command\n");
		return HAL_ERROR;
	}

	// Send command + data
	for (uint8_t board = 0; board < NUM_BOARDS; ++board)
	{
		if (batt_spi_tx(txBuffer, BUFF_SIZE) != HAL_OK)
		{
			ERROR_PRINT("Failed to transmit config to AMS board %u\n", board);
			return HAL_ERROR;
		}
	}

	return HAL_OK;
}

HAL_StatusTypeDef batt_write_config() {
	format_and_send_config(m_batt_configA, m_batt_configB);
    return HAL_OK;
}


static uint32_t PEC_count = 0;
static uint32_t last_PEC_tick = 0;

static HAL_StatusTypeDef batt_read_data(uint8_t first_byte, uint8_t second_byte, uint8_t* data_buffer, unsigned int response_size){
	const size_t BUFF_SIZE = COMMAND_SIZE + PEC_SIZE + ((response_size + PEC_SIZE) * NUM_BOARDS);
	const size_t DATA_START_IDX = COMMAND_SIZE + PEC_SIZE;
	uint8_t rxBuffer[BUFF_SIZE];
	uint8_t txBuffer[BUFF_SIZE];
	memset(rxBuffer, 0xFF, BUFF_SIZE);
	memset(txBuffer, 0xFF, BUFF_SIZE);

	if(batt_spi_wakeup(true) != HAL_OK){
		ERROR_PRINT("Failed to wake up boards\n");
		return HAL_ERROR;
	}

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
		if (checkPECData(&(rxBuffer[startOfData]), response_size) != HAL_OK)
		{
			DEBUG_PRINT("PEC ERROR on board %d config (adbms6830) \r\n", board);
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


HAL_StatusTypeDef batt_read_config(uint8_t configA[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE], uint8_t configB[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE]) {
    const uint8_t response_buffer_size = NUM_BOARDS * NUM_LTC_CHIPS_PER_BOARD * BATT_CONFIG_SIZE;
	uint8_t response_bufferA[response_buffer_size];
	uint8_t response_bufferB[response_buffer_size];
	memset(response_bufferA, 0xFF, response_buffer_size);
	memset(response_bufferB, 0xFF, response_buffer_size);

	batt_read_data(RDCFGA_BYTE0, RDCFGA_BYTE1, response_bufferA, BATT_CONFIG_SIZE);
	batt_read_data(RDCFGB_BYTE0, RDCFGB_BYTE1, response_bufferB, BATT_CONFIG_SIZE);

    for(int board = 0; board < NUM_BOARDS; board++){
        for(int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++){
            int idx = (board * NUM_LTC_CHIPS_PER_BOARD) + chip;
            memcpy(&(configA[board][chip]), &(response_bufferA[idx * BATT_CONFIG_SIZE]), BATT_CONFIG_SIZE);
            memcpy(&(configB[board][chip]), &(response_bufferB[idx * BATT_CONFIG_SIZE]), BATT_CONFIG_SIZE);
        }
    }

	return HAL_OK;
}

HAL_StatusTypeDef batt_verify_config() {
    uint8_t config_bufferA[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};
    uint8_t config_bufferB[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE] = {0};
	if(batt_read_config(config_bufferA, config_bufferB) != HAL_OK){
		ERROR_PRINT("Failed to read config");
		return HAL_ERROR;
	}

    vTaskDelay(T_REFUP_MS); // Let core state machine transition from STANDBY to REFUP
	
	// Verify was set correctly
	for(int board = 0; board < NUM_BOARDS; board++) {
		for(int ltc_chip = 0; ltc_chip < NUM_LTC_CHIPS_PER_BOARD; ltc_chip++) {
			DEBUG_PRINT("\r\nConfig Read A, Board %d, Chip %d: ", board, ltc_chip); 
			for(int buff_byte = 0; buff_byte < BATT_CONFIG_SIZE; buff_byte++) {
				DEBUG_PRINT("0x%02X ", config_bufferA[board][ltc_chip][buff_byte]);
				// if((m_batt_configA[board][ltc_chip][buff_byte] & 0x7) != (config_bufferA[board][ltc_chip][buff_byte] & 0x7)) { // Only care to check the REFON, ADC_OPT, SWTRD are set, not the GPIO pin states
				// 	ERROR_PRINT("\n ERROR: board: %d, ltc_chip: %d, buff_byte %d, %u != %u  \n", board, ltc_chip, buff_byte, m_batt_configA[board][ltc_chip][buff_byte], config_bufferA[board][ltc_chip][buff_byte]);
				// 	return HAL_ERROR;
				// }
			}

			DEBUG_PRINT("\r\nConfig Read B, Board %d, Chip %d: ", board, ltc_chip); 
			for(int buff_byte = 0; buff_byte < BATT_CONFIG_SIZE; buff_byte++) {
				DEBUG_PRINT("0x%02X ", config_bufferB[board][ltc_chip][buff_byte]);
				// if((m_batt_configB[board][ltc_chip][buff_byte] & 0x7) != (config_bufferB[board][ltc_chip][buff_byte] & 0x7)) { // Only care to check the REFON, ADC_OPT, SWTRD are set, not the GPIO pin states
				// 	ERROR_PRINT("\n ERROR: board: %d, ltc_chip: %d, buff_byte %d, %u != %u  \n", board, ltc_chip, buff_byte, m_batt_configB[board][ltc_chip][buff_byte], config_bufferB[board][ltc_chip][buff_byte]);
				// 	return HAL_ERROR;
				// }
			}
			DEBUG_PRINT("\n");
		}
	}
	return HAL_OK;
}


HAL_StatusTypeDef batt_readBackCellVoltage(float *cell_voltage_array, voltage_operation_t voltage_operation) {
    uint8_t cell_index = 0;

    const uint8_t rd_cmds[6][2] = {
        { RDCVA_BYTE0, RDCVA_BYTE1 },
        { RDCVB_BYTE0, RDCVB_BYTE1 },
        { RDCVC_BYTE0, RDCVC_BYTE1 },
        { RDCVD_BYTE0, RDCVD_BYTE1 },
        { RDCVE_BYTE0, RDCVE_BYTE1 },
        { RDCVF_BYTE0, RDCVF_BYTE1 },
    };

    for (int block = 0; block < 6; block++)
    {
        uint8_t adc_vals[NUM_BOARDS * VOLTAGE_BLOCK_SIZE] = {0};

		if (batt_spi_wakeup(false /* not sleeping*/)) {
            return HAL_ERROR;
		}

        if (batt_read_data(rd_cmds[block][0], rd_cmds[block][1], adc_vals, VOLTAGE_BLOCK_SIZE) != HAL_OK) {
            DEBUG_PRINT("ADBMS6830 voltage read failed (block %u)\r\n", block);
            return HAL_ERROR;
        }

        // Each block contains 3 cell readings
        for (int cell = 0; cell < 3; cell++) {
			// Only populate the cells that are wired on this board.
			// (ADBMS6830 has up to 16 cell inputs; we may use fewer.)
			if (cell_index >= CELLS_PER_BOARD)
                break;

            for (int board = 0; board < NUM_BOARDS; board++)
            {
                const size_t data_idx =
                    board * VOLTAGE_BLOCK_SIZE + (cell * CELL_VOLTAGE_SIZE_BYTES);
					
				// adc_vals[data_idx] as LSB and adc_vals[data_idx+1] as MSB
                uint16_t adc = ((uint16_t)adc_vals[data_idx + 1] << 8) |
                                adc_vals[data_idx];

                // Convert to volts
                // From Table 104: Cell Voltage = ADC × 150 uV + 1.5 V
                float voltage = (adc * 0.000150f) + 1.5f;

                const size_t global_cell =
					board * CELLS_PER_BOARD + cell_index;

                cell_voltage_array[global_cell] = voltage;

				if(voltage_operation == OPEN_WIRE)
				{
					open_wire_failure[global_cell].num_times_consec = 0;
				}
            }

            cell_index++;
        }
    }

    return HAL_OK;
}

void batt_set_temp_config(size_t channel) {
	const uint8_t gpioPins = channel;
	for (int board = 0; board < NUM_BOARDS; board++) {
		for (int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++) {
			m_batt_configA[board][chip][3] = gpioPins & 0x0F;
		}
	}
}

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
		case(ADSV): 
		{
			command_byte_low = ADSV_BYTE0;
			command_byte_high = ADSV_BYTE1;
			break;
		}
		case(ADAX_DOWN):
		{
			command_byte_low = ADAX_BYTE0;
			command_byte_high = ADAX_BYTE1(0);
			break;
		}
		case(ADAX_UP):
		{
			command_byte_low = ADAX_BYTE0;
			command_byte_high = ADAX_BYTE1(1);
			break;
		}
		case(ADAX2):
		{
			command_byte_low = ADAX2_BYTE0;
			command_byte_high = ADAX2_BYTE1;
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

HAL_StatusTypeDef batt_read_thermistors(size_t channel, float *cell_temp_array) {
	// adc values for one AUX block from all boards
	uint8_t adc_vals[NUM_BOARDS * AUX_BLOCK_SIZE] = {0};

	if (batt_read_data(RDAUXB_BYTE0, RDAUXB_BYTE1, adc_vals, AUX_BLOCK_SIZE) != HAL_OK) {
        DEBUG_PRINT("ADBMS6830 GPIO (thermistor) read failed for channel %zu\r\n", channel);
        return HAL_ERROR;
    }

	// Process the readings for each board
    for (int board = 0; board < NUM_BOARDS; board++) {
        size_t tempIdx = board * SEGMENT_THERMISTORS_AMS1 + channel;

        // GPIO 5 is in AUXB register (bytes 2-3)
        const size_t boardStartIdx = board * AUX_BLOCK_SIZE;
        uint16_t adcCounts = ((uint16_t)adc_vals[boardStartIdx + 3] << 8) |
                            adc_vals[boardStartIdx + 2];

        // Convert ADC code to volts
        // From Table 104: GPIO Voltage = ADC × 150 uV + 1.5 V
        float voltageThermistor = (adcCounts * 0.000150f) + 1.5f;
        cell_temp_array[tempIdx] = batt_convert_voltage_to_temp(voltageThermistor);
    }

	return HAL_OK;
}

void batt_set_balancing_cell (int board, int chip, int cell) {
    if (cell < 8) { // 8 bits per byte in the register
        SETBIT(m_batt_configB[board][chip][4], cell);
    } else {
		SETBIT(m_batt_configB[board][chip][5], cell - 8);
	}
}

void batt_unset_balancing_cell (int board, int chip, int cell) {
    if (cell < 8) { // 8 bits per byte in the register
        CLEARBIT(m_batt_configB[board][chip][4], cell);
    } else {
        CLEARBIT(m_batt_configB[board][chip][5], cell - 8);
	}
}

bool batt_get_balancing_cell_state(int board, int chip, int cell) {
    if (cell < 8) { // 8 bits per byte in the register
        return GETBIT(m_batt_configB[board][chip][4], cell);
    } else {
        return GETBIT(m_batt_configB[board][chip][5], cell - 8);
	}
}

HAL_StatusTypeDef batt_config_discharge_timer(DischargeTimerLength length) {
    if (length >= INVALID_DT_TIME) {
        return HAL_ERROR;
    }

	static const uint8_t discharge_minutes[] = {
        [DT_OFF]    = 0,
        [DT_30_SEC] = 1,
        [DT_1_MIN]  = 1,
        [DT_2_MIN]  = 2,
        [DT_3_MIN]  = 3,
        [DT_4_MIN]  = 4,
        [DT_5_MIN]  = 5,
        [DT_10_MIN] = 10,
        [DT_15_MIN] = 15,
        [DT_20_MIN] = 20,
    };

	uint8_t minutes = discharge_minutes[length];

	if (minutes > 63) {
        return HAL_ERROR;
	}

    for (int board = 0; board < NUM_BOARDS; board++) {
		for(int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++) {	
			m_batt_configB[board][chip][3] &= 0x00; // Clear previous timer settings
			m_batt_configB[board][chip][3] |= DTMEN(1);
			// m_batt_configB[board][chip][3] |= DTRNG(0);

			if (minutes > 0) {
                // Store timeout
                m_batt_configB[board][chip][3] |= (minutes & 0x3F);
            }
		}
    }

    return HAL_OK;
}

#endif