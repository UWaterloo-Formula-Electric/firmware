#ifndef DAC_H
#define DAC_H

#include "driver/spi_master.h"

#define BYTE0 0x02
#define TXLENGTH 24
#define VREF 3300.0
#define MAXSTEPS12 2048
#define MAXSTEPS8 255

typedef enum dacID{
    throttleA_ID = 0,
    throttleB_ID,
    brakePos_ID,
    steerRaw_ID,
}dacID;

int setDacVoltage(float voltage);
int set6551Voltage (float voltage, dacID id);

extern spi_device_handle_t throttle_A;
extern spi_device_handle_t throttle_B;
extern spi_device_handle_t brake_pos;
extern spi_device_handle_t steer_raw;

#endif