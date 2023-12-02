#ifndef USER_INIT_H
#define USER_INIT_H
#include "driver/spi_master.h"

void taskRegister (void);
esp_err_t CAN_init (void);
esp_err_t spi_init(void);

#endif // USER_INIT_H