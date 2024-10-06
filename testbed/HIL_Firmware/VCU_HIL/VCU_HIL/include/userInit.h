#ifndef USER_INIT_H
#define USER_INIT_H

//SPI pin configurations for vcu hil
#define SPI_MOSI_PIN 41
#define SPI_CLK_PIN  39
#define THROTTLE_A_CS_PIN 37
#define THROTTLE_B_CS_PIN 36
#define BRAKE_POS_CS_PIN 38
#define STEER_RAW_CS_PIN 35
#define HZ_PER_MHZ (1000*1000)
#define NOT_USED -1
#define MAX_SPI_QUEUE_SIZE 7

void taskRegister (void);
esp_err_t CAN_init (void);
esp_err_t spi_init (void);

#endif/*USER_INIT_H*/
