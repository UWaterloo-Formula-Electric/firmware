#ifndef DAC_H
#define DAC_H

#include "driver/spi_master.h"

#define BYTE_0 0x02
#define TX_LENGTH 24
#define V_REF 3300.0
#define MAX_STEPS_12 2048
#define MAX_STEPS_8 255
#define MESSAGE_STATUS 0x8060F02
#define BRAKE_POS_IS_SET 2
#define STEER_RAW_IS_SET 4
#define THROTTLE_A_IS_SET 8
#define THROTTLE_B_IS_SET 16

typedef enum DacId_E{
    DacId_throttleA = 0,
    DacId_throttleB,
    DacId_brakePos,
    DacId_steerRaws,
} DacId_E;

int setDacVoltage(float voltage);
int set6551Voltage (float voltage, DacId_E id);

extern spi_device_handle_t throttle_A;
extern spi_device_handle_t throttle_B;
extern spi_device_handle_t brake_pos;
extern spi_device_handle_t steer_raw;

#endif