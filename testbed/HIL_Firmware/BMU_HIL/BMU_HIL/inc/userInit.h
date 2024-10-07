#ifndef USER_INIT_H
#define USER_INIT_H

/***********************************
********** DEFINITIONS *************
************************************/
#define CAN_TX GPIO_NUM_12
#define CAN_RX GPIO_NUM_13

/***********************************
***** FUNCTION DECLERATIONS ********
************************************/
esp_err_t CAN_init (void);
esp_err_t SPI_init(void);
esp_err_t GPIO_init (void);

#endif/*USER_INIT_H*/