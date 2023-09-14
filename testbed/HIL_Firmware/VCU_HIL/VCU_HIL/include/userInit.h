#ifndef USER_INIT_H
#define USER_INIT_H

//SPI pin configurations for vcu hil
#define SPI_MOSI_PIN 41
#define SPI_CLK_PIN  39
#define THROTTLE_A_CS 37
#define THROTTLE_B_CS 36
#define BRAKE_POS_CS 38
#define STEER_RAW_CS 35
#define HZ_PER_MHZ 1000*1000

void taskRegister (void);
int CAN_init (void);
int spi_init (void);

//end of USER_INIT_H
#endif