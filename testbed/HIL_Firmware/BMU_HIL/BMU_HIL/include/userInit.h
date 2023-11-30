#ifndef USER_INIT_H
#define USER_INIT_H
#include "driver/spi_master.h"


#define CAN_TX GPIO_NUM_36
#define CAN_RX GPIO_NUM_35


void taskRegister (void);
esp_err_t CAN_init (void);
esp_err_t spi_init(void);
#endif // USER_INIT_H