#ifndef USER_INIT_H
#define USER_INIT_H

//SPI Config

//SPI Stuff
#define SPI_SDO_PIN 39
#define SPI_SCK_PIN  38

//GPIO Pins
#define BR_IR_TEMP 10
#define BR_PRESS 11
#define WHE_ENC 17
#define BC_ROT_ENC 18
#define HF_EFF 24
#define MC_FLOW 25


//CAN Pins
#define CAN_TX GPIO_NUM_12
#define CAN_RX GPIO_NUM_13

//Constants
#define HZ_PER_MHZ (1000*1000) //Converts frequency
#define NOT_USED -1 
#define MAX_SPI_QUEUE_SIZE 7

void taskRegister (void);
esp_err_t CAN_init (void);
esp_err_t spi_init (void);

#endif/*USER_INIT_H*/