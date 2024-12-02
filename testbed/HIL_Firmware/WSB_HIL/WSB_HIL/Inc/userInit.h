#ifndef USER_INIT_H
#define USER_INIT_H

//SPI Config

//SPI Stuff
#define SPI_SDO_PIN 1
#define SPI_SCK_PIN 2

//GPIO Pins
#define BR_IR_TEMP 17
#define BR_PRESS 18
#define WHE_ENC 9
#define BC_ROT_ENC 10
#define HL_EFF 33
#define MC_FLOW 34

//CAN Pins
#define CAN_TX GPIO_NUM_12
#define CAN_RX GPIO_NUM_13

//Constants
#define HZ_PER_MHZ (1000*1000) //Converts frequency
#define NOT_USED -1 
#define MAX_SPI_QUEUE_SIZE 7

typedef enum PwmChannel_E {
    PwmChannel_HallEff = 0,
} PwmChannel_E;

typedef enum PwmTimer_E {
    PwmTimer_HallEff = 0,
} PwmTimer_E;

void taskRegister (void);
esp_err_t CAN_init (void);
esp_err_t spi_init (void);
esp_err_t pwm_init (void);

#endif/*USER_INIT_H*/