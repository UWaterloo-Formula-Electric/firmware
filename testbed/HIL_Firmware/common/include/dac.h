#ifndef DAC_H
#define DAC_H

#include "driver/spi_master.h"

#define BYTE0 0x02
#define TXLENGTH 24
#define VREF 3300.0
#define MAXSTEPS12 2048
#define MAXSTEPS8 255
#define MESSAGE_STATUS 0x8060F02
#define BRAKE_POS_IS_SET 2
#define STEER_RAW_IS_SET 4
#define THROTTLE_A_IS_SET 8
#define THROTTLE_B_IS_SET 16

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