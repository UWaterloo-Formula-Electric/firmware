#ifndef USER_INIT_H
#define USER_INIT_H

//SPI pin configurations for esp32s2, check documentation for further explanation
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/hw-reference/esp32s2/user-guide-s2-devkitc-1.html
#define PIN_NUM_MOSI 41
#define PIN_NUM_CLK  39
#define THROTTLE_A_CS 37
#define THROTTLE_B_CS 36
#define BRAKE_POS_CS 38
#define STEER_RAW_CS 35

void taskRegister (void);
int CAN_init (void);
int spi_init (void);

//end of USER_INIT_H
#endif