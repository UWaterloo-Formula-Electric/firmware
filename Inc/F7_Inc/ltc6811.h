// Copyright @ 2015 Waterloo Hybrid

#ifndef LTC6811_H

#define LTC6811_H

#include <stdint.h>

// The following defines change depending on battery box layout
#define NUM_BOARDS                  5   // Number of AMS boards in system
#define CELLS_PER_BOARD             12  // Number of valid cells per board, starting from the most negative terminal
#define THERMISTORS_PER_BOARD       8   // Number of thermistors attached to each AMS, starting from A0

// The following defines are always fixed due to AMS architecture, DO NOT CHANGE
#define VOLTAGE_BLOCKS_PER_BOARD    4   // Number of voltage blocks per AMS board
#define VOLTAGES_PER_BLOCK          3   // Number of voltage reading per block

/* Functions */
void batt_init_board(uint16_t address);
void batt_write_config_to_board(uint16_t address);
void batt_read_config(uint8_t* rxBuffer, uint8_t address);
int32_t batt_read_voltage_block(uint16_t address, uint8_t block, uint16_t *adc_vals);       // Reads voltage block of specified AMS board
int32_t batt_read_temperature_block(uint16_t address, uint8_t block, uint16_t *adc_vals);   // Reads temperature block of specified AMS board
int32_t batt_start_voltage_measurement(uint16_t address);
int32_t batt_start_temp_measurement(uint16_t address, uint32_t channel);
void batt_clear_registers();
int32_t batt_poll_adc_state(uint8_t address);
void batt_unset_balancing_cell (int board, int cell);
void batt_set_balancing_cell (int board, int cell);

/* Helpers */
void batt_gen_pec(uint8_t * arrdata, unsigned int dataSizeInBytes, uint8_t * pec);
void batt_spi_wakeup();

#endif /* end of include guard: LTC6811_H */
