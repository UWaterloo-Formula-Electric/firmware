/*
 * ltc6811.c
 *
 *  Created on: May 2, 2019
 *      Author: Richard Matthews
 *
 *  Copyright @ 2019 Waterloo Formula Electric
 */
#include "ltc6811.h"
#include "bsp.h"
#include "math.h"
#include "stdbool.h"
#include "errorHandler.h"
#include "debug.h"
#include "freertos.h"
#include "task.h"

// The following defines are always fixed due to AMS architecture, DO NOT CHANGE
#define VOLTAGE_BLOCKS_PER_BOARD    4   // Number of voltage blocks per AMS board
#define VOLTAGES_PER_BLOCK          3   // Number of voltage reading per block
#define TEMP_CHANNELS_PER_BOARD     12
#define VOLTAGE_MEASURE_DELAY_MS    2   // Length of time for voltage measurements to finish
#define VOLTAGE_MEASURE_DELAY_EXTRA_US 400 // Time to add on to ms delay for measurements to finsh
#define TEMP_MEASURE_DELAY_US 405 // Time for measurements to finsh

/* 6804 Commands */
// Address is specified in the first byte of the command. Each command is 2 bytes.
// See page 46-47 of http://cds.linear.com/docs/en/datasheet/680412fb.pdf
// If not using address mode, set address to 0
/*#define USE_ADDRESS_MODE*/
// For testing, allow using address mode, but assume address is 0

#ifdef USE_ADDRESS_MODE
#define WRCFG_BYTE0 (0x80)
#define WRCFG_BYTE1 0x01

#define RDCFG_BYTE0 (0x80)
#define RDCFG_BYTE1 0x02

#define RDCVA_BYTE0 (0x80)
#define RDCVA_BYTE1 0x04

#define RDCVB_BYTE0 (0x80)
#define RDCVB_BYTE1 0x06

#define RDCVC_BYTE0 (0x80)
#define RDCVC_BYTE1 0x08

#define RDCVD_BYTE0 (0x80)
#define RDCVD_BYTE1 0x0a

#define PLADC_BYTE0  (0x87)
#define PLADC_BYTE1 0x14

#define CLRSTAT_BYTE0 (0x87)
#define CLRSTAT_BYTE1 0x13

#define CLRCELL_BYTE0 (0x87)
#define CLRCELL_BYTE1 0x11

#define CLRAUX_BYTE0 (0x87)
#define CLRAUX_BYTE1 0x12

// For reading auxiliary register group A
#define RDAUXA_BYTE0 (0x80)
#define RDAUXA_BYTE1 0x0c

// For reading auxiliary register group B
#define RDAUXB_BYTE0 (0x80)
#define RDAUXB_BYTE1 0x0e

// Use normal MD (7kHz), Discharge not permission, all channels and GPIO 1 & 2
#define ADCVAX_BYTE0 (0x85)
#define ADCVAX_BYTE1 0x6f

// Use normal MD (7kHz), Discharge not permission, all channels
#define ADCV_BYTE0 (0x83)
#define ADCV_BYTE1 0x60

// Use fast MD (27kHz), Discharge not permission, all channels and GPIO 1 & 2
#define ADAX_BYTE0 (0x85)
#define ADAX_BYTE1 0x60

#define RDSTATB_BYTE0 (0x80)
#define RDSTATB_BYTE1 0x12

#define ADOW_BYTE0 (0x85)
#define ADOW_BYTE1(PUP) (0x28 | ((PUP)<<6))

#else // defined(USE_ADDRES_MODE)

#define WRCFG_BYTE0 (0x00)
#define WRCFG_BYTE1 0x01

#define RDCFG_BYTE0 (0x00)
#define RDCFG_BYTE1 0x02

#define RDCVA_BYTE0 (0x00)
#define RDCVA_BYTE1 0x04

#define RDCVB_BYTE0 (0x00)
#define RDCVB_BYTE1 0x06

#define RDCVC_BYTE0 (0x00)
#define RDCVC_BYTE1 0x08

#define RDCVD_BYTE0 (0x00)
#define RDCVD_BYTE1 0x0a

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
#define ADAX_BYTE1 0x60

#define RDSTATB_BYTE0 (0x00)
#define RDSTATB_BYTE1 0x12

#define ADOW_BYTE0 (0x05)
#define ADOW_BYTE1(PUP) (0x28 | ((PUP)<<6))

#endif // defined(USE_ADDRESS_MODE)

#define PEC_INIT_VAL 0x0010

/* Semantic Defines to make code easier to read */
#define BITS_PER_BYTE 8
#define GETBIT(value,bit) ((value>>(bit))&1)
#define CLEARBIT(value, bit) value &= ~(1 << (bit))
#define SETBIT(value, bit) value |= (1 << (bit))
#define ASSIGNBIT(value, newvalue, bit) value = ((value) & ~(1 << (bit))) | ((newvalue) << (bit))

#define BATT_CONFIG_SIZE 6
#define COMMAND_SIZE 2
#define PEC_SIZE 2
#define VOLTAGE_BLOCK_SIZE 6
#define AUX_BLOCK_SIZE 6
#define CELL_VOLTAGE_SIZE_BYTES 2
#define STATUS_SIZE 6
#define JUNK_SIZE 1

#define STATUS_INDEX_END 4
#define STATUS_INDEX_START 2

#define TEMP_ADC_IDX_LOW 2
#define TEMP_ADC_IDX_HIGH 3

#define GPIO_CONVERSION_TIME 800        // See datasheet for min conversion times w/ 7kHz ADC
#define BATT_CONVERSION_TIME 3000
#define T_SLEEP_US           2000000    // The LTC sleeps in 2 seconds
#define T_WAKE_MS            1        // The LTC wakes in 300 us, but since systick is 1 KHz just round up to 1 ms
#define T_READY_US           10 // The time to bring up ISOSPI bus if already in standby
#define T_IDLE_MS            5 // Time for SPI bus to go to idle state (5.5 ms)

// Config Byte 0 options
// CFGR0 RD/WR GPIO5 GPIO4 GPIO3 GPIO2 GPIO1 REFON SWTRD ADCOPT

#define ADC_OPT(en) (en << 0) // Since we're using the normal 7kHz mode
#define SWTRD(en) (en << 1) // We're not using the software time
#define REFON(en) (en << 2)
#define GPIO5_POS 7
#define GPIO4_POS 6
#define GPIO3_POS 5
#define GPIO2_POS 4
#define GPIO1_POS 3

/** Voltage constants in 100uV steps **/
#define VUV 0x658 // based on: (VUV + 1) * 16 * 100uV and target VUV of 2.6V
#define VOV 0x8CA // based on: (VOV) * 16 * 100uV and target VOV of 3.6V

#define VOLTAGE_REGISTER_COUNTS_PER_VOLT 10000 // 1 LSB is 100uV

// Configuration array
static uint8_t m_batt_config[NUM_BOARDS][BATT_CONFIG_SIZE] = {0};

/* HSPI send/receive function */
#define HSPI_TIMEOUT 15
HAL_StatusTypeDef spi_tx_rx(uint8_t * tdata, uint8_t * rbuffer,
                            unsigned int len) {
    //Make sure rbuffer is large enough for the sending command + receiving bytes
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&ISO_SPI_HANDLE, tdata, rbuffer, len, HSPI_TIMEOUT);
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_SET);
    return status;
}

HAL_StatusTypeDef batt_spi_tx(uint8_t *txBuffer, size_t len)
{
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&ISO_SPI_HANDLE, txBuffer, len, HSPI_TIMEOUT);
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_SET);
    return status;
}

void batt_init_board_config(uint16_t board) 
{
    m_batt_config[board][0] =   (1 << GPIO1_POS) |  // Mux always goes to GPIO1
                            REFON(1) |          // Turn on referemce 
                            ADC_OPT(0);         // We use fast ADC speed so this has to be 0
    m_batt_config[board][4] = 0x00;                 // Disable the discharge bytes for now
    m_batt_config[board][5] = 0x00;
}

/*
 * Generates a 15bit PEC for the message defined for data.
 * Uses CRC with polynomial:
 *   x^15 + x^14 + x^10 + x^8 + x^7 + x^4 + x^3 + 1
 *
 * NOTE: Wondering how this works? Beats me. Check out the LTC6804 datasheet.
 */
void batt_gen_pec(uint8_t * arrdata, unsigned int num_bytes, uint8_t * pecAddr) {
    unsigned char in0;
    unsigned char in3;
    unsigned char in4;
    unsigned char in7;
    unsigned char in8;
    unsigned char in10;
    unsigned char in14;
    int i;
    int n;

    unsigned short pec = PEC_INIT_VAL;
    for (n = 0; n < num_bytes; n++) {
        uint8_t data = arrdata[n];
        for (i = 0; i < 8; i++, data = data << 1) {
            unsigned char din = GETBIT(data, 7);

            in0 = din ^ GETBIT(pec, 14);
            in3 = in0 ^ GETBIT(pec, 2);
            in4 = in0 ^ GETBIT(pec, 3);
            in7 = in0 ^ GETBIT(pec, 6);
            in8 = in0 ^ GETBIT(pec, 7);
            in10 = in0 ^ GETBIT(pec, 9);
            in14 = in0 ^ GETBIT(pec, 13);

            ASSIGNBIT(pec, in14, 14);
            ASSIGNBIT(pec, GETBIT(pec, 12), 13);
            ASSIGNBIT(pec, GETBIT(pec, 11), 12);
            ASSIGNBIT(pec, GETBIT(pec, 10), 11);
            ASSIGNBIT(pec, in10, 10);
            ASSIGNBIT(pec, GETBIT(pec,8), 9);
            ASSIGNBIT(pec, in8, 8);
            ASSIGNBIT(pec, in7, 7);
            ASSIGNBIT(pec, GETBIT(pec, 5), 6);
            ASSIGNBIT(pec, GETBIT(pec, 4), 5);
            ASSIGNBIT(pec, in4, 4);
            ASSIGNBIT(pec, in3, 3);
            ASSIGNBIT(pec, GETBIT(pec, 1), 2);
            ASSIGNBIT(pec, GETBIT(pec, 0), 1);
            ASSIGNBIT(pec, in0, 0);
        }
    }

    // Stuff the LSB with a 0, since pec are only 15 bits but we have 2 bytes to send
    pec = (pec << 1);
    CLEARBIT(pec, 0);

    pecAddr[0] = (pec >> 8) & 0xff;
    pecAddr[1] = pec & 0xff;
}


/*
 * Check the PEC on received data
 * @param rxBuffer: buffer holding the read data and PEC
 * @param dataSize: length of the data the PEC is calculated on
 */
HAL_StatusTypeDef checkPEC(uint8_t *rxBuffer, size_t dataSize)
{
    uint8_t pec[2];
    batt_gen_pec(rxBuffer, dataSize, pec);
    uint32_t pec_loc = dataSize;
    if (pec[0] == rxBuffer[pec_loc] && pec[1] == rxBuffer[pec_loc + 1])
    {
        return HAL_OK;
    } else {
        return HAL_ERROR;
    }
}

static void fillDummyBytes(uint8_t * buf, uint32_t length)
{
    for (uint32_t dbyte = 0; dbyte < length; dbyte++)
    {
        buf[dbyte] = 0xFF;
    }
}

/* delay function for wakeup */
void delay_us(uint32_t time_us)
{
	__HAL_TIM_SetCounter(&DELAY_TIMER,0);
	__HAL_TIM_SetAutoreload(&DELAY_TIMER,0xffff);
	HAL_TIM_Base_Start(&DELAY_TIMER);
	while(DELAY_TIMER_INSTANCE->CNT < time_us);
	HAL_TIM_Base_Stop(&DELAY_TIMER);
}

// Wake up the spi bus
// @param standby: Set to true if the device is in standby already, the device
// wakes faster in this case
uint32_t lastWakeup_ticks = 0;
int batt_spi_wakeup(bool sleeping)
{
    if (!sleeping) {
        // The boards have been initialized, so we're in the REFUP state
        if (xTaskGetTickCount() - lastWakeup_ticks <= pdMS_TO_TICKS(T_IDLE_MS)) {
            // SPI bus already up
            return 0;
        }
    }

    // Send some dummy bytes to wake up SPI/LTC
    uint8_t dummy = 0xFF;

    // Wake up the serial interface on device S1.
    if (batt_spi_tx(&dummy, JUNK_SIZE))
    {
        ERROR_PRINT("Failed to wakeup batt spi\n");
        return 1;
    }

    if (sleeping) {
        HAL_Delay(T_WAKE_MS);
    } else {
        delay_us(T_READY_US);
    }

    // For large stacks where some devices may go to the IDLE state after waking
    if (batt_spi_tx(&dummy, JUNK_SIZE))
    {
        ERROR_PRINT("Failed to wakeup batt spi\n");
        return 1;
    }
    lastWakeup_ticks = xTaskGetTickCount();

    if (sleeping) {
        HAL_Delay(T_WAKE_MS);
    } else {
        delay_us(T_READY_US);
    }

    return 0;
}

// Write a broadcast command and pec to a tx buffer
// Make sure txBuffer is big enough
HAL_StatusTypeDef batt_format_broadcast_command(uint8_t cmdByteLow, uint8_t cmdByteHigh, uint8_t *txBuffer)
{
    txBuffer[0] = cmdByteLow;
    txBuffer[1] = cmdByteHigh;
    batt_gen_pec(txBuffer, COMMAND_SIZE, &(txBuffer[COMMAND_SIZE]));

    return HAL_OK;
}

HAL_StatusTypeDef batt_write_config()
{
    const size_t BUFF_SIZE = COMMAND_SIZE + PEC_SIZE + ((BATT_CONFIG_SIZE + PEC_SIZE) * NUM_BOARDS);
    uint8_t txBuffer[BUFF_SIZE];

    if (batt_format_broadcast_command(WRCFG_BYTE0, WRCFG_BYTE1, txBuffer) != HAL_OK) {
        ERROR_PRINT("Failed to send write config command\n");
        return HAL_ERROR;
    }

    // Fill data
    for (int board = 0; board < NUM_BOARDS; board++) {
        size_t startByte = COMMAND_SIZE + PEC_SIZE + board * (BATT_CONFIG_SIZE + PEC_SIZE);

        for (int dbyte = 0; dbyte < BATT_CONFIG_SIZE; dbyte++)
        {
            txBuffer[startByte + dbyte]
                = m_batt_config[board][dbyte];
        }

        // Data pec
        batt_gen_pec(m_batt_config[board], BATT_CONFIG_SIZE,
                     &(txBuffer[COMMAND_SIZE + PEC_SIZE + board * (BATT_CONFIG_SIZE+PEC_SIZE) + BATT_CONFIG_SIZE]));

    }

    // Send command + data
    if (batt_spi_tx(txBuffer, BUFF_SIZE) != HAL_OK)
    {
        ERROR_PRINT("Failed to transmit config to AMS board\n");
        return HAL_ERROR;
    }

    return 0;
}

/*
 * Read data from the daisy chain
 * @param: cmdByteLow and High: cmd to send
 * @param: dataOutBuffer buffer to put read data into (PECs removed after
 * verification)
 * @param: readSizePerBoard: amount of data to read per board
 */
HAL_StatusTypeDef batt_read_data(uint8_t cmdByteLow, uint8_t cmdByteHigh, uint8_t *dataOutBuffer, size_t readSizePerBoard)
{
    const size_t BUFF_SIZE = COMMAND_SIZE + PEC_SIZE + NUM_BOARDS * (readSizePerBoard + PEC_SIZE);
    const size_t DATA_START_IDX = COMMAND_SIZE + PEC_SIZE;
    uint8_t rxBuffer[BUFF_SIZE];
    uint8_t txBuffer[BUFF_SIZE];

    if (batt_format_broadcast_command(cmdByteLow, cmdByteHigh, txBuffer) != HAL_OK) {
        return HAL_ERROR;
    }

    fillDummyBytes(&(txBuffer[DATA_START_IDX]), NUM_BOARDS * (readSizePerBoard + PEC_SIZE));

    if (spi_tx_rx(txBuffer, rxBuffer, BUFF_SIZE) != HAL_OK) {
        ERROR_PRINT("Failed to send read data command\n");
        return HAL_ERROR;
    }

    for (int board = 0; board < NUM_BOARDS; board++) {
        int boardDataStartIdx = DATA_START_IDX + (PEC_SIZE*readSizePerBoard) * board;
        if (checkPEC(&(rxBuffer[boardDataStartIdx]), readSizePerBoard) != HAL_OK)
        {
            ERROR_PRINT("PEC mismatch\n");
            return HAL_ERROR;
        }

        // Copy data out
        for (int i = 0; i<readSizePerBoard; i++) {
            dataOutBuffer[board*readSizePerBoard + i] = rxBuffer[boardDataStartIdx + i];
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_read_config(uint8_t *rxBuffer)
{
    if (batt_read_data(RDCFG_BYTE0, RDCFG_BYTE1, rxBuffer, BATT_CONFIG_SIZE) != HAL_OK)
    {
        ERROR_PRINT("Failed to read config\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_init()
{
    uint8_t configReadBuffer[BATT_CONFIG_SIZE * NUM_BOARDS];

    for (int board = 0; board < NUM_BOARDS; board++) {
        batt_init_board_config(board);
    }

    batt_spi_wakeup(true /* sleeping */);

    if(batt_write_config() != HAL_OK) {
        ERROR_PRINT("Failed to write batt config to boards\n");
        return HAL_ERROR;
    }

    if(batt_read_config(configReadBuffer) != HAL_OK) {
        ERROR_PRINT("Failed to read batt config from boards\n");
        return HAL_ERROR;
    }

    for (int board = 0; board < NUM_BOARDS; board++) {
        for (int i = 0; i < BATT_CONFIG_SIZE; i++) {
            if (configReadBuffer[i] != m_batt_config[board][i]) {
                ERROR_PRINT("Config read (%i) doesn't match written (%d) for board %d\n",
                            configReadBuffer[i], m_batt_config[board][i], board);
                return HAL_ERROR;
            }
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_read_cell_voltages(float *cell_voltage_array)
{
    const size_t TX_BUFF_SIZE = COMMAND_SIZE + PEC_SIZE;
    uint8_t txBuffer[TX_BUFF_SIZE];

    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    if (batt_format_broadcast_command(ADCV_BYTE0, ADCV_BYTE1, txBuffer) != HAL_OK)
    {
        ERROR_PRINT("Failed to format read voltage command\n");
        return HAL_ERROR;
    }

    if (batt_spi_tx(txBuffer, TX_BUFF_SIZE) != HAL_OK)
    {
        ERROR_PRINT("Failed to transmit read voltage command\n");
        return HAL_ERROR;
    }

    vTaskDelay(VOLTAGE_MEASURE_DELAY_MS);
    delay_us(VOLTAGE_MEASURE_DELAY_EXTRA_US);
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    // Voltage values for one block from all boards
    uint8_t adc_vals[VOLTAGE_BLOCK_SIZE * NUM_BOARDS] = {0};

    for (int block = 0; block < VOLTAGE_BLOCKS_PER_BOARD; block++) {

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
        } else {
            ERROR_PRINT("Attempt to access unkown voltage block\n");
            return HAL_ERROR;
        }

        if (batt_read_data(cmdByteLow, cmdByteHigh, adc_vals, VOLTAGE_BLOCK_SIZE) != HAL_OK) {
            ERROR_PRINT("Failed to read voltage block %d\n", block);
            return HAL_ERROR;
        }

        // Have to read 3 battery voltages from each cell voltage register
        for (int board = 0; board < NUM_BOARDS; board++) {
            for (int cvreg = 0; cvreg < VOLTAGES_PER_BLOCK; cvreg ++)
            {
                size_t index = cvreg*CELL_VOLTAGE_SIZE_BYTES;
                size_t cellIdx = cvreg + board * CELLS_PER_BOARD + VOLTAGES_PER_BLOCK*block;
                uint16_t temp = ((uint16_t) (adc_vals[(index + 1)] << 8 | adc_vals[index]));
                cell_voltage_array[cellIdx] = ((float)temp) / VOLTAGE_REGISTER_COUNTS_PER_VOLT;
            }
        }
    }

    return HAL_OK;
}

// input voltage is in 100uV
// output temp is in degrees C
float batt_convert_voltage_to_temp(uint16_t voltage) {
    // directly using the formulae from 
    // http://www.murata.com/~/media/webrenewal/support/library/catalog/products/thermistor/ntc/r44e.ashx

    const float r0 = 10e3;
    const float t0 = 298.15;
    const float B = 3380;
    const float vt = 3.0;
    float v = 0.0001 * (float) voltage;
    if (vt == v) {
        return NAN;
    }
    // http://www.wolframalpha.com/input/?i=rearrange+v+%3D+r%2F(r%2Br0)*v0+for+r
    float r = r0 * v / (vt - v);
    float temperature = 1./(logf(r/r0)/B + 1./t0) - 273.15f;
    return temperature;
}

HAL_StatusTypeDef batt_read_cell_temps_single_channel(size_t channel, float *cell_temp_array)
{
    const size_t TX_BUFF_SIZE = COMMAND_SIZE + PEC_SIZE;
    uint8_t txBuffer[TX_BUFF_SIZE];

    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    // Validate parameters
    if(c_assert(channel < TEMP_CHANNELS_PER_BOARD))
    {
        return HAL_ERROR;
    }

    for (int board = 0; board < NUM_BOARDS; board++)
    {
        // Set the external MUX to channel we want to read. MUX pin is selected via GPIO2, GPIO3, GPIO4, LSB first.
        m_batt_config[board][0] = (channel << GPIO1_POS) | REFON(1) | ADC_OPT(0);
    }

    if (batt_write_config() != HAL_OK)
    {
        ERROR_PRINT("Failed to setup mux for temp reading\n");
        return HAL_ERROR;
    }

    if (batt_format_broadcast_command(ADAX_BYTE0, ADAX_BYTE1, txBuffer) != HAL_OK)
    {
        ERROR_PRINT("Failed to format read temp command\n");
        return HAL_ERROR;
    }

    if (batt_spi_tx(txBuffer, TX_BUFF_SIZE) != HAL_OK)
    {
        ERROR_PRINT("Failed to transmit read temp command\n");
        return HAL_ERROR;
    }

    delay_us(TEMP_MEASURE_DELAY_US);
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    // adc values for one block from all boards
    uint8_t adc_vals[AUX_BLOCK_SIZE * NUM_BOARDS] = {0};

    for (int board = 0; board < NUM_BOARDS; board++) {
        size_t cellIdx = board * CELLS_PER_BOARD + channel;
        // We only use one GPIO input to measure temperatures, so pick that out
        uint16_t temp = ((uint16_t) (adc_vals[TEMP_ADC_IDX_HIGH] << 8
                                     | adc_vals[TEMP_ADC_IDX_LOW]));
        cell_temp_array[cellIdx] = batt_convert_voltage_to_temp(temp);
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_read_cell_temps(float *cell_temp_array)
{
    for (int channel = 0; channel < TEMP_CHANNELS_PER_BOARD; channel++)
    {
        if (batt_read_cell_temps_single_channel(channel, cell_temp_array) != HAL_OK)
        {
            return HAL_ERROR;
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

    return HAL_ERROR;
}
