#ifndef DAC_H
#define DAC_H

#include "driver/spi_master.h"
#include "driver/dac_oneshot.h"

//the 8 MSB on the DAC6551 is constant
//the power down mode connects a 100kOhm pulldow resistor to Vout
#define BYTE_0 0x02
#define MAX_SPI_QUEUE_LENGTH 24
#define V_REF_MV 3300.0
#define MAX_12_BIT_VAL 2048
#define MAX_8_BIT_VAL 255
#define MESSAGE_STATUS_CAN_ID 0x8060F02
#define BRAKE_PRES_RAW_DAC_SET 1
#define BRAKE_POS_DAC_SET 2
#define STEER_RAW_DAC_SET 4
#define THROTTLE_A_DAC_SET 8
#define THROTTLE_B_DAC_SET 16
#define EXTENDED_MSG 1              //CAN message has extended ID
#define CAN_MSG_DATA_SIZE 1     //size in bytes

typedef enum DacId_E{
    DacId_ThrottleA = 0,
    DacId_ThrottleB,
    DacId_BrakePos,
    DacId_SteerRaw,
} DacId_E;

esp_err_t setDacVoltage(float voltage);
esp_err_t set6551Voltage (float voltage, DacId_E id);

extern spi_device_handle_t throttle_A;
extern spi_device_handle_t throttle_B;
extern spi_device_handle_t brake_pos;
extern spi_device_handle_t steer_raw;
extern dac_oneshot_handle_t brake_pres_raw;

#endif/*DAC_H*/
