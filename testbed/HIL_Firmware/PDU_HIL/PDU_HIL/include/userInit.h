#ifndef USER_INIT_H
#define USER_INIT_H

#include "driver/spi_master.h"

#define CAN_TX GPIO_NUM_12
#define CAN_RX GPIO_NUM_13
#define DC_DC_TOGGLE_PIN 1
#define POT_SPI_CLK_PIN 39
#define POT_MOSI_PIN 40
#define POT_CS_PIN 41
#define POT_NSHUTDOWN_PIN 17 // Active Low
#define POT_NSET_MID_PIN 18  // Active Low
#define HZ_PER_MHZ (1000*1000)
#define NOT_USED -1
#define MAX_SPI_QUEUE_SIZE 7

extern spi_device_handle_t pot;

void taskRegister (void);
esp_err_t CAN_init (void);
esp_err_t spi_init(void);
void pot_init(void);
void pdu_input_init(void);

#endif/*USER_INIT_H*/
