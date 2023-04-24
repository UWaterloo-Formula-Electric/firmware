#ifndef USER_INIT_H
#define USER_INIT_H

#define PIN_NUM_CLK 39
#define PIN_NUM_MOSI 40
#define POT_CS 41

#include "driver/spi_master.h"

extern spi_device_handle_t pot;

void taskRegister (void);
int CAN_init (void);
int spi_init(void);


//end of USER_INIT_H
#endif
