#ifndef LTC_COMMON_H
#define LTC_COMMON_H
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ltc_chip.h"

// The following defines are always fixed due to AMS architecture, DO NOT CHANGE
#define TEMP_CHANNELS_PER_BOARD     14
#define VOLTAGE_MEASURE_DELAY_MS    2   // Length of time for voltage measurements to finish
#define VOLTAGE_MEASURE_DELAY_EXTRA_US 400 // Time to add on to ms delay for measurements to finsh
#define TEMP_MEASURE_DELAY_US 405 // Time for measurements to finsh
#define MUX_MEASURE_DELAY_US  1 // Time for Mux to switch


#define PEC_INIT_VAL 0x0010

/* Semantic Defines to make code easier to read */
#define US_TO_MS(us) ((uint64_t)(us) / 1000)
#define BITS_PER_BYTE 8
#define GETBIT(value,bit) ((value>>(bit))&1)
#define CLEARBIT(value, bit) value &= ~(1 << (bit))
#define SETBIT(value, bit) value |= (1 << (bit))
#define ASSIGNBIT(value, newvalue, bit) value = ((value) & ~(1 << (bit))) | ((newvalue) << (bit))

#define BATT_CONFIG_SIZE 6    // Size of Config per Register
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
#define T_IDLE_US            4400 // Time for ISOSPI bus to go to idle state (min 4.4ms, typ 5.5 ms)
#define T_REFUP_MS           4.4 // Takes 4.4 ms for reference to power up

// Config Byte 0 options
// CFGR0 RD/WR GPIO5 GPIO4 GPIO3 GPIO2 GPIO1 REFON SWTRD ADCOPT

#define ADC_OPT(en) ((en) << 0) // Since we're using the normal 7kHz mode
#define SWTRD(en) ((en) << 1) // We're not using the software time
#define REFON(en) ((en) << 2)


#if LTC_CHIP == LTC_CHIP_6804 || LTC_CHIP == LTC_CHIP_6812
// These GPIO pins are on Register B
#define GPIO9_POS 3
#define GPIO8_POS 2
#define GPIO7_POS 1
#define GPIO6_POS 0

//These GPIO pins are on Register A
#define GPIO5_POS 7
#define GPIO4_POS 6
#define GPIO3_POS 5
#define GPIO2_POS 4
#define GPIO1_POS 3

#elif LTC_CHIP == ADBMS_CHIP_6830B
// These GPIO pins are on Register B
#define GPIO10_POS 1
#define GPIO9_POS 0

//These GPIO pins are on Register A
#define GPIO8_POS 7
#define GPIO7_POS 6
#define GPIO6_POS 5
#define GPIO5_POS 4
#define GPIO4_POS 3
#define GPIO3_POS 2
#define GPIO2_POS 1
#define GPIO1_POS 0

/* Table 93 RDSTATE group E, STER4: GPI[8:1]; bits 0..4 = GPI1..GPI5 (after GPIO/mux set via WRCFGA) */
#define RDSTATE_STER4_IDX           4u
#define RDSTATE_GPI1_TO_GPI5_MASK   0x1Fu
#define rdstate_gpi1_to_gpi5(ster4) ((uint8_t)((ster4) & RDSTATE_GPI1_TO_GPI5_MASK))

/* RDSTATC group C: thermal shutdown flag in STER5 (byte 5), bit 2 */
#define RDSTATC_STER5_IDX            5u
#define RDSTATC_THERMAL_SHUTDOWN_BIT 2u
#define rdstatc_thermal_shutdown(statc_row) \
(GETBIT((statc_row)[RDSTATC_STER5_IDX], RDSTATC_THERMAL_SHUTDOWN_BIT))
#endif

/** Voltage constants in 100uV steps **/
#define VUV 0x1CA // based on: (VUV * 16 * 150uV) + 1.5V and target VUV of 2.6V
#define VOV 0x36B // based on: (VOV * 16 * 150uV) + 1.5V and target VOV of 3.6V


#define VOLTAGE_REGISTER_COUNTS_PER_VOLT 15000 // 1 LSB is 150uV

#define VOLTAGES_PER_BLOCK          3   // Number of voltage reading per block

HAL_StatusTypeDef batt_format_command(uint8_t cmdByteLow, uint8_t cmdByteHigh, uint8_t *txBuffer);
HAL_StatusTypeDef batt_format_write_config_command(uint8_t cmdByteLow, uint8_t cmdByteHigh, uint8_t *txBuffer, uint8_t writeData[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE], uint8_t writeDataSize);
void batt_gen_pec(uint8_t * arrdata, unsigned int num_bytes, uint8_t * pecAddr);
void batt_gen_pec_data(uint8_t * arrdata, unsigned int num_bytes, uint8_t * pecAddr, uint8_t cmd_counter);
HAL_StatusTypeDef batt_spi_tx(uint8_t *txBuffer, size_t len);
HAL_StatusTypeDef spi_tx_rx(uint8_t * tdata, uint8_t * rbuffer, unsigned int len);
void fillDummyBytes(uint8_t * buf, uint32_t length);
HAL_StatusTypeDef checkPEC(uint8_t *rxBuffer, size_t dataSize);
HAL_StatusTypeDef checkPECData(uint8_t *rxBuffer, size_t dataSize);
int batt_spi_wakeup(bool sleeping);
float batt_convert_voltage_to_temp(float voltage); 
void long_delay_us(uint32_t time_us);
void delay_us(const uint16_t time_us);

#endif
