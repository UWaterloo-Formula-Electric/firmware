#ifndef DAC_H
#define DAC_H

#include "driver/spi_master.h"

#define BYTE0 0x02
#define TXLENGTH 24
#define VREF 3300.0
#define MAXSTEPS12 2047

//SPI pin configurations for esp32s2, check documentation for further explanation
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/hw-reference/esp32s2/user-guide-s2-devkitc-1.html
#define PIN_NUM_MISO 41 //not used
#define PIN_NUM_MOSI 41
#define PIN_NUM_CLK  39
#define THROTTLE_A_CS 37
#define THROTTLE_B_CS 36
#define BRAKE_POS_CS 38
#define STEER_RAW_CS 35

//#define PIN_NUM_DC   4
//#define PIN_NUM_RST  5
//#define PIN_NUM_BCKL 6

#define MAXSTEPS8 255

int setDacVoltage(float voltage);
int deleteChannel(uint32_t channel);
int set6551Voltage (float voltage, uint32_t id);
int spi_init (void);

typedef enum dacID{
    throttleA_ID = 0,
    throttleB_ID,
    brakePos_ID,
    steerRaw_ID,
}dacID;

static spi_device_handle_t throttle_A;
static spi_device_handle_t throttle_B;
static spi_device_handle_t brake_pos;
static spi_device_handle_t steer_raw;

#endif