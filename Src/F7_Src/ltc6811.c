/*
 * ltc6811.c
 *
 *  Created on: Mar 31, 2015
 *      Author: Kyle
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
#define VOLTAGE_MEASURE_DELAY_MS    2   // Length of time for voltage measurements to finish
#define VOLTAGE_MEASURE_DELAY_EXTRA_US 400 // Time to add on to ms delay for measurements to finsh
#define TEMP_MEASURE_DELAY_MS       2   // Length of time for temperature measurements to finish
#define TEMP_MEASURE_DELAY_EXTRA_US 400 // Time to add on to ms delay for measurements to finsh

/* 6804 Commands */
// Address is specified in the first byte of the command. Each command is 2 bytes.
// See page 46-47 of http://cds.linear.com/docs/en/datasheet/680412fb.pdf
// If not using address mode, set address to 0
#define USE_ADDRESS_MODE
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
#define STATUS_SIZE 6
#define JUNK_SIZE 1

#define STATUS_INDEX_END 4
#define STATUS_INDEX_START 2

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

/* Function Prototypes */
void batt_init_board(uint16_t address);
int batt_write_config_to_board(uint16_t address);
int batt_read_config(uint8_t* rxBuffer, uint8_t address);
int32_t batt_read_temperature_block(uint16_t address, uint8_t block, uint16_t *adc_vals);   // Reads temperature block of specified AMS board
int32_t batt_start_voltage_measurement();
int32_t batt_start_temp_measurement(uint16_t address, uint32_t channel);
void batt_clear_registers();
int32_t batt_poll_adc_state(uint8_t address);
void batt_unset_balancing_cell (int board, int cell);
void batt_set_balancing_cell (int board, int cell);

/* Helper prototypes */
void batt_gen_pec(uint8_t * arrdata, unsigned int dataSizeInBytes, uint8_t * pec);
int batt_spi_wakeup(bool sleeping);

/* delay function for wakeup */
void delay_us(uint32_t time_us)
{
	__HAL_TIM_SetCounter(&DELAY_TIMER,0);
	__HAL_TIM_SetAutoreload(&DELAY_TIMER,0xffff);
	HAL_TIM_Base_Start(&DELAY_TIMER);
	while(DELAY_TIMER_INSTANCE->CNT < time_us);
	HAL_TIM_Base_Stop(&DELAY_TIMER);
}

/* HSPI send/receive function */
#define HSPI_TIMEOUT 15
HAL_StatusTypeDef spi_tx_rx(uint8_t * tdata, uint8_t * rbuffer,
                            unsigned int len) {
    //This is the function that we will want to use normally, it transmits and receives at the same time (the way SPI operates).
    //Make sure rbuffer is large enough for the sending command + receiving bytes
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&ISO_SPI_HANDLE, tdata, rbuffer, len, HSPI_TIMEOUT);
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_SET);
    return status;
}

int32_t batt_spi_tx_rx(uint8_t * tdata, uint8_t * rbuffer, uint32_t len, int32_t check_pec, uint32_t rx_data_start, uint32_t rx_data_len) 
{
    // Zero buffer
    for (uint32_t i = 0; i < len; i++)
    {
        rbuffer[i] = 0;
    }

    if (spi_tx_rx (tdata, rbuffer, len) == HAL_OK) 
    {
        if (check_pec) 
        {
            uint8_t pec[2];
            batt_gen_pec(&rbuffer[rx_data_start], (rx_data_len), pec);
            uint32_t pec_loc = rx_data_start + rx_data_len;
            if (pec[0] == rbuffer[pec_loc] && pec[1] == rbuffer[pec_loc + 1]) 
            {
                return 0;
            } else {
                return 1;
            }
        }
        else 
        {
            return 0;
        }
    }
    else
    {
        return 1;
    }
}

int32_t batt_spi_tx_rx_no_pec(uint8_t * tdata, uint8_t * rbuffer, uint32_t len) 
{
    return batt_spi_tx_rx(tdata, rbuffer, len, 0, 0, 0);
}

int32_t batt_spi_tx_rx_pec(uint8_t * tdata, uint8_t * rbuffer, uint32_t len, uint32_t rx_data_start, uint32_t rx_data_len) 
{
    return batt_spi_tx_rx(tdata, rbuffer, len, 1, rx_data_start, rx_data_len);
}

static void fillDummyBytes(uint8_t * buf, uint32_t length) 
{
    for (uint32_t dbyte = 0; dbyte < length; dbyte++) 
    {
        buf[dbyte] = 0xFF;
    }
}

void batt_init_board(uint16_t address) 
{
    m_batt_config[address][0] =   (1 << GPIO1_POS) |  // Mux always goes to GPIO1
                            REFON(1) |          // Turn on referemce 
                            ADC_OPT(0);         // We use fast ADC speed so this has to be 0
    m_batt_config[address][4] = 0x00;                 // Disable the discharge bytes for now
    m_batt_config[address][5] = 0x00;
}


int batt_write_config_to_board(uint16_t address)
{
    const size_t BUF_SIZE = COMMAND_SIZE + PEC_SIZE + BATT_CONFIG_SIZE + PEC_SIZE;
    uint8_t rxBuffer[BUF_SIZE];
    uint8_t txBuffer[BUF_SIZE];

    txBuffer[0] = WRCFG_BYTE0;
    txBuffer[1] = WRCFG_BYTE1;
    batt_gen_pec(txBuffer, COMMAND_SIZE, &(txBuffer[COMMAND_SIZE]));

    for (int dbyte = 0; dbyte < BATT_CONFIG_SIZE; dbyte++) 
    {
        txBuffer[dbyte + COMMAND_SIZE + PEC_SIZE] = m_batt_config[address][dbyte];
    }

    // Data pec
    batt_gen_pec(m_batt_config[address], BATT_CONFIG_SIZE, &(txBuffer[COMMAND_SIZE + PEC_SIZE + BATT_CONFIG_SIZE]));

    // Transmit entire command + data
    if (batt_spi_tx_rx_no_pec(txBuffer, rxBuffer, BUF_SIZE))
    {
        ERROR_PRINT("Failed to transmit config to AMS board\n");
        return 1;
    }

    return 0;
}

int batt_read_config(uint8_t* dataBuffer, uint8_t address) {
    const size_t TX_SIZE = COMMAND_SIZE + PEC_SIZE;
    const size_t RX_SIZE = BATT_CONFIG_SIZE + PEC_SIZE;
    const size_t BUF_SIZE = TX_SIZE + RX_SIZE;

    // Read 6804 configuration registers
    uint8_t rxBuffer[BUF_SIZE];
    uint8_t txBuffer[BUF_SIZE];

    // Cmd
    txBuffer[0] = RDCFG_BYTE0;
    txBuffer[1] = RDCFG_BYTE1;
    batt_gen_pec(txBuffer, COMMAND_SIZE, &(txBuffer[COMMAND_SIZE]));

    // Fill rest with dummy bytes to drive clock signal
    fillDummyBytes(&txBuffer[TX_SIZE], RX_SIZE);

    // Transmit command, pec, and junk
    if (batt_spi_tx_rx_pec(txBuffer, rxBuffer, BUF_SIZE, TX_SIZE, BATT_CONFIG_SIZE))
    {
        ERROR_PRINT("Failed to read config from AMS board\n");
        return 1;
    }

    // We will copy the relevant data into the actual rxBuffer
    // Do this so the caller doesn't need to know info about the size of cmds
    for (int i = 0; i < BATT_CONFIG_SIZE; i++) {
        dataBuffer[i] = rxBuffer[i + TX_SIZE];
    }

    return 0;
}

HAL_StatusTypeDef batt_test_bus()
{
    DEBUG_PRINT("Testing bus\n");
    uint16_t board = 0;
    uint8_t configRead[BATT_CONFIG_SIZE] = {0};

    if (batt_spi_wakeup(true /*sleeping*/)) {
        return HAL_ERROR;
    }
    batt_init_board(0);
    if (batt_write_config_to_board(board)) {
        return HAL_ERROR;
    }
    DEBUG_PRINT("Wrote: %d %d %d\n", m_batt_config[board][0], m_batt_config[board][4], m_batt_config[board][5]);

    if (batt_read_config(configRead, board)) {
        return HAL_ERROR;
    }

    DEBUG_PRINT("Wrote: %d %d %d\n", m_batt_config[board][0], m_batt_config[board][4], m_batt_config[board][5]);

    DEBUG_PRINT("Read: %d %d %d\n", configRead[0], configRead[4], configRead[5]);
    return HAL_OK;
}

void batt_set_balancing_cell (int board, int cell) 
{
    if (cell < 8) // 8 bits per byte in the register
    {
        SETBIT(m_batt_config[board][4], cell);
    }
    else 
    {
        SETBIT(m_batt_config[board][5], cell - 8);
    }
}

void batt_unset_balancing_cell (int board, int cell) 
{
    if (cell < 8) // 8 bits per byte in the register
    {
        CLEARBIT(m_batt_config[board][4], cell);
    } 
    else 
    {
        CLEARBIT(m_batt_config[board][5], cell - 8);
    }
}

bool batt_get_balancing_cell_state(int board, int cell)
{
    if (cell < 8) // 8 bits per byte in the register
    {
        return GETBIT(m_batt_config[board][4], cell);
    }
    else
    {
        return GETBIT(m_batt_config[board][5], cell - 8);
    }
}

int32_t batt_poll_adc_state(uint8_t address) 
{
    const size_t TX_SIZE = COMMAND_SIZE + PEC_SIZE;
    const size_t RX_SIZE = JUNK_SIZE;
    const size_t BUF_SIZE = TX_SIZE + RX_SIZE;

    uint8_t txBuffer[BUF_SIZE];
    uint8_t rxBuffer[BUF_SIZE];

    txBuffer[0] = PLADC_BYTE0;
    txBuffer[1] = PLADC_BYTE1;
    batt_gen_pec(txBuffer, COMMAND_SIZE, &(txBuffer[COMMAND_SIZE]));

    fillDummyBytes(&txBuffer[TX_SIZE], RX_SIZE);

    batt_spi_tx_rx_no_pec(txBuffer, rxBuffer, BUF_SIZE);

    // Return true if the device is not busy converting
    return (rxBuffer[TX_SIZE] == 0xff);
}

int32_t batt_read_cell_voltage_block(uint8_t block, uint16_t *adc_vals)
{
    const uint8_t TX_SIZE = COMMAND_SIZE + PEC_SIZE;
    const uint8_t RX_SIZE = VOLTAGE_BLOCK_SIZE + PEC_SIZE;
    const uint8_t BUF_SIZE = TX_SIZE + RX_SIZE;
    uint8_t rxBuffer[BUF_SIZE];
    uint8_t txBuffer[BUF_SIZE];

    // Validate parameters
    if( c_assert(block < 4) ||
        c_assert(adc_vals))
    {
        return 1;
    }

    // Select appropriate bank
    if(block == 0)
    {
        txBuffer[0] = RDCVA_BYTE0;
        txBuffer[1] = RDCVA_BYTE1;
    }
    else if(block == 1)
    {
        txBuffer[0] = RDCVB_BYTE0;
        txBuffer[1] = RDCVB_BYTE1;
    }
    else if(block == 2)
    {
        txBuffer[0] = RDCVC_BYTE0;
        txBuffer[1] = RDCVC_BYTE1;
    }
    else if(block == 3)
    {
        txBuffer[0] = RDCVD_BYTE0;
        txBuffer[1] = RDCVD_BYTE1;
    }
    else
    {
        return 1;
    }

    // Generate packet with Packet Error Correction
    batt_gen_pec(txBuffer, COMMAND_SIZE, &(txBuffer[COMMAND_SIZE]));
    fillDummyBytes(&txBuffer[TX_SIZE], RX_SIZE);

    batt_spi_tx_rx_pec(txBuffer, rxBuffer, BUF_SIZE, TX_SIZE, VOLTAGE_BLOCK_SIZE);

    // Have to read 3 battery voltages from each cell voltage register
    for (int cvreg = 0; cvreg < VOLTAGES_PER_BLOCK; cvreg ++)
    {
        uint16_t index = TX_SIZE + cvreg*2;
        adc_vals[cvreg] = ((unsigned short) (rxBuffer[(index + 1)] << 8 | rxBuffer[index]));
    }

    return 0;
}

/*int32_t batt_read_temperature_block(uint16_t address, uint8_t block, uint16_t *adc_vals) */
/*{*/
    /*const uint8_t TX_SIZE = COMMAND_SIZE + PEC_SIZE;*/
    /*const uint8_t RX_SIZE = VOLTAGE_BLOCK_SIZE + PEC_SIZE;*/
    /*const uint8_t BUF_SIZE = TX_SIZE + RX_SIZE;*/
    /*uint8_t rxBuffer[BUF_SIZE];*/
    /*uint8_t txBuffer[BUF_SIZE];*/

    /*// Validate parameters*/
    /*if( c_assert(address < NUM_BOARDS) ||*/
        /*c_assert(block < 2) ||*/
        /*c_assert(adc_vals))*/
    /*{*/
        /*return 1;*/
    /*}*/

    /*// Select appropriate block*/
    /*if(block == 0)*/
    /*{*/
        /*txBuffer[0] = RDAUXA_BYTE0(address);*/
        /*txBuffer[1] = RDAUXA_BYTE1;*/
    /*}*/
    /*else if(block == 1)*/
    /*{*/
        /*txBuffer[0] = RDAUXB_BYTE0(address);*/
        /*txBuffer[1] = RDAUXB_BYTE1;*/
    /*}*/
    /*else*/
    /*{*/
        /*return 1;*/
    /*}*/

    /*batt_gen_pec(txBuffer, COMMAND_SIZE, &(txBuffer[COMMAND_SIZE]));*/
    /*fillDummyBytes(&txBuffer[TX_SIZE], RX_SIZE);*/

    /*if (batt_spi_tx_rx_pec(txBuffer, rxBuffer, BUF_SIZE, TX_SIZE, VOLTAGE_BLOCK_SIZE))*/
    /*{*/
        /*ERROR_PRINT("Failed to read temperature block\n");*/
        /*return 1;*/
    /*}*/

    /*// Have to read 3 thermistor voltages from each auxiliary register*/
    /*for (int cvreg = 0; cvreg < 3; cvreg ++) */
    /*{*/
        /*uint16_t index = TX_SIZE + cvreg*2;*/
        /*adc_vals[cvreg] = ((unsigned short) (rxBuffer[(index + 1)] << 8 | rxBuffer[index]));*/
    /*}*/
    /*return 0;*/
/*}*/

/*int32_t batt_start_temp_measurement(uint16_t address, uint32_t channel)*/
/*{*/
    /*const uint32_t TX_SIZE = COMMAND_SIZE + PEC_SIZE;*/
    /*const uint32_t BUF_SIZE = TX_SIZE;*/

    /*// Validate parameters*/
    /*if(c_assert(channel < 12))*/
    /*{*/
        /*return 1;*/
    /*}*/

    /*// Set the external MUX to channel we want to read. MUX pin is selected via GPIO2, GPIO3, GPIO4, LSB first.*/
    /*switch(channel)*/
    /*{*/
        /*case 0: m_batt_config[address][0] = (1 << GPIO1_POS) | (0 << GPIO2_POS) | (0 << GPIO3_POS) | (0 << GPIO4_POS) | REFON(1) | ADC_OPT(0); break;*/
        /*case 1: m_batt_config[address][0] = (1 << GPIO1_POS) | (1 << GPIO2_POS) | (0 << GPIO3_POS) | (0 << GPIO4_POS) | REFON(1) | ADC_OPT(0); break;*/
        /*case 2: m_batt_config[address][0] = (1 << GPIO1_POS) | (0 << GPIO2_POS) | (1 << GPIO3_POS) | (0 << GPIO4_POS) | REFON(1) | ADC_OPT(0); break;*/
        /*case 3: m_batt_config[address][0] = (1 << GPIO1_POS) | (1 << GPIO2_POS) | (1 << GPIO3_POS) | (0 << GPIO4_POS) | REFON(1) | ADC_OPT(0); break;*/
        /*case 4: m_batt_config[address][0] = (1 << GPIO1_POS) | (0 << GPIO2_POS) | (0 << GPIO3_POS) | (1 << GPIO4_POS) | REFON(1) | ADC_OPT(0); break;*/
        /*case 5: m_batt_config[address][0] = (1 << GPIO1_POS) | (1 << GPIO2_POS) | (0 << GPIO3_POS) | (1 << GPIO4_POS) | REFON(1) | ADC_OPT(0); break;*/
        /*case 6: m_batt_config[address][0] = (1 << GPIO1_POS) | (0 << GPIO2_POS) | (1 << GPIO3_POS) | (1 << GPIO4_POS) | REFON(1) | ADC_OPT(0); break;*/
        /*case 7: m_batt_config[address][0] = (1 << GPIO1_POS) | (1 << GPIO2_POS) | (1 << GPIO3_POS) | (1 << GPIO4_POS) | REFON(1) | ADC_OPT(0); break;*/
    /*}*/

    /*// Send over via SPI*/
    /*if (batt_write_config_to_board(address))*/
    /*{*/
        /*return 1;*/
    /*}*/

    /*uint8_t txBuffer[BUF_SIZE];*/
    /*uint8_t rxBuffer[BUF_SIZE];*/
    /*txBuffer[0] = ADAX_BYTE0(address);*/
    /*txBuffer[1] = ADAX_BYTE1;*/

    /*// send ADAX and PEC over SPI*/
    /*batt_gen_pec(txBuffer, COMMAND_SIZE, &(txBuffer[COMMAND_SIZE]));*/
    /*if (batt_spi_tx_rx_no_pec(txBuffer, rxBuffer, BUF_SIZE))*/
    /*{*/
        /*return 1;*/
    /*}*/

    /*return 0;*/
/*}*/

int32_t batt_start_voltage_measurement()
{
    const uint8_t TX_SIZE = COMMAND_SIZE + PEC_SIZE;
    const uint8_t BUF_SIZE = TX_SIZE;

    uint8_t txBuffer[BUF_SIZE];
    uint8_t rxBuffer[BUF_SIZE];
    txBuffer[0] = ADCV_BYTE0;
    txBuffer[1] = ADCV_BYTE1;

    // send ADCV and PEC over SPI
    batt_gen_pec(txBuffer, COMMAND_SIZE, &(txBuffer[COMMAND_SIZE]));
    if (batt_spi_tx_rx_no_pec(txBuffer, rxBuffer, BUF_SIZE))
    {
        ERROR_PRINT("Failed to start voltage measurement\n");
        return 1;
    }

    return 0;
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
    uint8_t rxBuffer;

    // Wake up the serial interface on device S1.
    if (batt_spi_tx_rx_no_pec(&dummy, &rxBuffer, JUNK_SIZE))
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
    if (batt_spi_tx_rx_no_pec(&dummy, &rxBuffer, JUNK_SIZE))
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

// input voltage is in 100uV
// output temp is in degrees C
float batt_convert_voltage_to_temp(int voltage) {
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

HAL_StatusTypeDef batt_read_cell_voltages(float *cell_voltage_array)
{
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    /*
     * Voltage Readings
     */
    if (batt_start_voltage_measurement(0))
    {
        ERROR_PRINT("Failed to start voltage measure\n");
        return HAL_ERROR;
    }

    vTaskDelay(VOLTAGE_MEASURE_DELAY_MS);
    delay_us(VOLTAGE_MEASURE_DELAY_EXTRA_US);
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    if (c_assert(batt_poll_adc_state(0))) {
        ERROR_PRINT("adc still busy\r\n");
        return HAL_ERROR;
    }

    uint16_t adc_vals[VOLTAGES_PER_BLOCK] = {0}; // Temp buffer
    for (int board = 0; board < NUM_BOARDS; board++)
    {
        for (int block = 0; block < VOLTAGE_BLOCKS_PER_BOARD; block++) {

            // Read vals from board
            if (batt_read_cell_voltage_block(block, adc_vals)) {
                ERROR_PRINT("Failed to read voltage block %i\n", block);
                return HAL_ERROR;
            }

            // copy to voltage array
            int cell_index = 0;

            for (int voltage = 0; voltage < VOLTAGES_PER_BLOCK; voltage++) {
                cell_index = board*CELLS_PER_BOARD + block*VOLTAGES_PER_BLOCK + voltage;

                // Put reading into array, convert from 100s of uV to float in volts
                cell_voltage_array[cell_index]
                    = ((float)adc_vals[voltage]) / VOLTAGE_REGISTER_COUNTS_PER_VOLT;
            }

        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_read_cell_temps(float *cell_temp_array)
{
    /*
     * Temperature readings
     */
    /*for (int i = 0; i < THERMISTORS_PER_BOARD; i++) {*/
        /*// TODO: Do we need this wakeup? The boards should be up after the*/
        /*// voltage measure, but maybe theres a chance the task gets*/
        /*// interrupted in the middle*/
        /*if (batt_spi_wakeup(false [> not sleeping <]))*/
        /*{*/
            /*return HAL_ERROR;*/
        /*}*/
        /*if (batt_start_temp_measurement(0, i))*/
        /*{*/
            /*ERROR_PRINT("Failed to start temp measure\n");*/
            /*return HAL_ERROR;*/
        /*}*/
        /*vTaskDelay(TEMP_MEASURE_DELAY_MS);*/
        /*if (batt_spi_wakeup(false [> not sleeping <]))*/
        /*{*/
            /*return HAL_ERROR;*/
        /*}*/
        /*if (c_assert(batt_poll_adc_state(0))) {*/
            /*return HAL_ERROR;*/
        /*}*/

        /*uint16_t adc_vals[3] = {0};*/
        /*if (c_assert(!batt_read_temperature_block(0, 0, adc_vals))) {*/
            /*return HAL_ERROR;*/
        /*}*/
        /*// TODO: Since there's less thermistors then cells, figure out what*/
        /*// to do for the array*/
        /*cell_temp_array[0 * THERMISTORS_PER_BOARD + i] = batt_convert_voltage_to_temp(adc_vals[0]);*/
        /*// printf("B%dT%d,%hx\r\n", board, i, adc_vals[0]);*/
    /*}*/

    return HAL_OK;
}

/*
 *
 * Public Functions
 *
 */

HAL_StatusTypeDef batt_read_cell_voltages_and_temps(float *cell_voltage_array, float *cell_temp_array)
{
    if (batt_read_cell_voltages(cell_voltage_array) != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef batt_balance_cell(int cell)
{
    if (c_assert(cell < NUM_VOLTAGE_CELLS))
    {
        return HAL_ERROR;
    }

    int boardIdx = cell / CELLS_PER_BOARD;
    int amsCellIdx = cell - (boardIdx * CELLS_PER_BOARD);

    batt_set_balancing_cell(boardIdx, amsCellIdx);

    return HAL_OK;
}

bool batt_is_cell_balancing(int cell)
{
    if (c_assert(cell < NUM_VOLTAGE_CELLS))
    {
        return false;
    }

    int boardIdx = cell / CELLS_PER_BOARD;
    int amsCellIdx = cell - (boardIdx * CELLS_PER_BOARD);

    return batt_get_balancing_cell_state(boardIdx, amsCellIdx);
}

HAL_StatusTypeDef batt_write_balancing_config()
{
    for (int board = 0; board < NUM_BOARDS; board++) {
        if (batt_write_config_to_board(board)) {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_unset_balancing_all_cells()
{
    for (int board = 0; board < NUM_BOARDS; board++) {
        for (int cell = 0; cell < CELLS_PER_BOARD; cell++) {
            batt_unset_balancing_cell(board, cell);
        }
    }

    return HAL_OK;
}

int32_t sendADOWCMD(uint16_t address, uint8_t pullup)
{
    const uint8_t TX_SIZE = COMMAND_SIZE + PEC_SIZE;
    const uint8_t BUF_SIZE = TX_SIZE;

    uint8_t txBuffer[BUF_SIZE];
    uint8_t rxBuffer[BUF_SIZE];

    c_assert(pullup==0 || pullup==1);

    txBuffer[0] = ADOW_BYTE0;
    txBuffer[1] = ADOW_BYTE1(pullup);

    // send ADCV and PEC over SPI
    batt_gen_pec(txBuffer, COMMAND_SIZE, &(txBuffer[COMMAND_SIZE]));
    if (batt_spi_tx_rx_no_pec(txBuffer, rxBuffer, BUF_SIZE))
    {
        ERROR_PRINT("Failed to send ADOW command\n");
        return 1;
    }

    return 0;
}

HAL_StatusTypeDef checkForOpenCircuit()
{
    for (int board = 0; board < NUM_BOARDS; board++)
    {
        if (batt_spi_wakeup(false /* not sleeping*/)) {
            return HAL_ERROR;
        }

        if (sendADOWCMD(0 /* address is always zero for now since we are using daisy chain */,
                        1)) {
            return HAL_ERROR;
        }
        if (sendADOWCMD(0 /* address is always zero for now since we are using daisy chain */,
                        1)) {
            return HAL_ERROR;
        }

    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_init()
{
    uint8_t configRead[BATT_CONFIG_SIZE] = {0};

    if (batt_spi_wakeup(true /* sleeping*/)) {
        return HAL_ERROR;
    }

    for (int board = 0; board < NUM_BOARDS; board++)
    {
        batt_init_board(board);
        if (batt_write_config_to_board(board))
        {
            ERROR_PRINT("Failed to init board %d\n", board);
            return HAL_ERROR;
        }

        if (batt_read_config(configRead, board)) {
            return HAL_ERROR;
        }

        for (int i=0; i<BATT_CONFIG_SIZE; i++) {
            if (configRead[i] != m_batt_config[board][i]) {
                ERROR_PRINT("Config read doesn't match config written, [%d] r %d != w %d\n", i, configRead[i], m_batt_config[board][i]);
                return HAL_ERROR;
            }
        }
    }

    return HAL_OK;
}
