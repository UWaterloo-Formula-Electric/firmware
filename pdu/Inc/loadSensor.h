/**
 *******************************************************************************
 * @file    loadSensor.h
 * @author	Rijin + Jacky Lim
 * @date    Oct 2024
 *
 *******************************************************************************
 */
#ifndef LOAD_SENSOR_H_
#define LOAD_SENSOR_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include <stdint.h>

typedef enum PDU_Channels_t {
    Pump_1_Channel,     // Channel 1
    Pump_2_Channel,     // Channel 2
    CDU_Channel,        // Channel 3
    BMU_Channel,        // Channel 4
    WSB_Channel,        // Channel 5
    TCU_Channel,        // Channel 6
    Brake_Light_Channel,// Channel 7
    ACC_Fans_Channel,   // Channel 8
    INV_Channel,        // Channel 9
    Radiator_Channel,   // Channel 10
    AUX_1_Channel,      // Channel 11
    AUX_2_Channel,      // Channel 12
    AUX_3_Channel,      // Channel 13
    AUX_4_Channel,      // Channel 14
    NUM_PDU_CHANNELS
} PDU_Channels_t;

typedef struct {
    float Pump_1_Channel_A;
    float Pump_2_Channel_A;
    float CDU_Channel_A;
    float BMU_Channel_A;
    float WSB_Channel_A;
    float TCU_Channel_A;
    float Brake_Light_Channel_A;
    float ACC_Fans_Channel_A;
    float INV_Channel_A;
    float Radiator_Channel_A;
    float AUX_1_Channel_A;
    float AUX_2_Channel_A;
    float AUX_3_Channel_A;
    float AUX_4_Channel_A;
} ChannelMeas_t;

// This union allows us to print out the data easily
typedef union {
    ChannelMeas_t meas_s;
    float meas_a[NUM_PDU_CHANNELS];
} channelMeas_u;

extern const char *channelNames[NUM_PDU_CHANNELS];
extern channelMeas_u channelCurrents;
extern uint32_t rawADC3Buffer[NUM_PDU_CHANNELS];
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define MUX_CHANNELS        7

#define LOAD_SENSOR_TASK_INTERVAL_MS        100
#define SETTLING_TIME_AFTER_CHANNEL_CHANGE_LOW_TO_HIGH_US 20    // Taking max interval as per datasheet (needs to be checked)
#define SETTLING_TIME_AFTER_CHANNEL_CHANGE_HIGH_TO_LOW_US 20    // Taking max interval as per datasheet (needs to be checked)

#define ADC3_TO_AMPS_DIVIDER      74.47272727                   // 1 / ([(3.3/4096) / 330] * 5500)

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
void printRawChannelADCVals(void);
void printChannelCurrent(void);

#endif /* LOAD_SENSOR_H_ */