#ifndef LTC_COMMON_H
#define LTC_COMMON_H
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"

// The following defines are always fixed due to AMS architecture, DO NOT CHANGE
#define TEMP_CHANNELS_PER_BOARD     16
#define VOLTAGE_MEASURE_DELAY_MS    2   // Length of time for voltage measurements to finish
#define VOLTAGE_MEASURE_DELAY_EXTRA_US 400 // Time to add on to ms delay for measurements to finsh
#define TEMP_MEASURE_DELAY_US 405 // Time for measurements to finsh
#define MUX_MEASURE_DELAY_US  1 // Time for Mux to switch


#define PEC_INIT_VAL 0x0010

/* Semantic Defines to make code easier to read */
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
#define T_IDLE_MS            5 // Time for SPI bus to go to idle state (5.5 ms)
#define T_REFUP_MS           6 // Takes 5.5 ms for reference to power up

// Config Byte 0 options
// CFGR0 RD/WR GPIO5 GPIO4 GPIO3 GPIO2 GPIO1 REFON SWTRD ADCOPT

#define ADC_OPT(en) ((en) << 0) // Since we're using the normal 7kHz mode
#define SWTRD(en) ((en) << 1) // We're not using the software time
#define REFON(en) ((en) << 2)


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


/** Voltage constants in 100uV steps **/
#define VUV 0x658 // based on: (VUV + 1) * 16 * 100uV and target VUV of 2.6V
#define VOV 0x8CA // based on: (VOV) * 16 * 100uV and target VOV of 3.6V


#define VOLTAGE_REGISTER_COUNTS_PER_VOLT 10000 // 1 LSB is 100uV

#define VOLTAGES_PER_BLOCK          3   // Number of voltage reading per block

HAL_StatusTypeDef batt_format_command(uint8_t cmdByteLow, uint8_t cmdByteHigh, uint8_t *txBuffer);
void batt_gen_pec(uint8_t * arrdata, unsigned int num_bytes, uint8_t * pecAddr);
HAL_StatusTypeDef batt_spi_tx(uint8_t *txBuffer, size_t len);
HAL_StatusTypeDef spi_tx_rx(uint8_t * tdata, uint8_t * rbuffer, unsigned int len);
void fillDummyBytes(uint8_t * buf, uint32_t length);
    void wakeup_idle();
    void wakeup_sleep();
HAL_StatusTypeDef checkPEC(uint8_t *rxBuffer, size_t dataSize);
int batt_spi_wakeup(bool sleeping);
float batt_convert_voltage_to_temp(float voltage); 
void delay_us(uint32_t time_us);

#endif
