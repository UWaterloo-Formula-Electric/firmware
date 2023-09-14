#ifndef USER_INIT_H
#define USER_INIT_H

#include "driver/spi_master.h"

#define POT_SPI_CLK_PIN 39
#define PIN_NUM_MOSI 40
#define POT_CS 41
#define POT_NSHUTDOWN 17 // Active Low
#define POT_NSET_MID 18  // Active Low
#define HZ_PER_MHZ 1000*1000

extern spi_device_handle_t pot;

void taskRegister (void);
int CAN_init (void);
int spi_init(void);
void pot_init(void);
void pdu_input_init(void);


//end of USER_INIT_H
#endif
