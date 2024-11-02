#ifndef SENSORS_H

#define SENSORS_H

/*
 * !!IMPORTANT!!
 * These are numbered to match which ADC channel they are connected to, don't
 * change the order or add channels unless you know what you're doing!!
 */

/* This is enum is for all the PDU output channels */
typedef enum PDU_Channels_t {
    Pump_1_Channel,     // Channel 1
    Pump_2_channel,     // Channel 2
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

/* This enum is for the sense lines on ADC1 */
typedef enum PDU_ADC1_Channels_t {
    I_Main_Channel,             // ADC Channel 4
    V_Main_Channel,             // ADC Channel 5
    V_Lipo_Channel,             // ADC Channel 6
    I_DCDC_Channel,             // ADC Channel 7
    V_DCDC_Channel,             // ADC Channel 14
    Lipo_Therm_Channel,         // ADC Channel 15
    NUM_ADC1_CHANNELS           // Total = 6
} PDU_ADC1_Channels_t;

const char *channelNames[NUM_PDU_CHANNELS];

#define SENSOR_READ_PERIOD_MS 500
#define PDU_CURRENT_PUBLISH_PERIOD_MS 1000
#define PDU_POWER_STATES_PUBLISH_PERIOD_MS 5000
#define PDU_FUSE_STATUS_PUBLISH_PERIOD_MS 10000
#define PDU_PUBLISH_PERIOD_MS 1000

/*
 * Sensor Valid Range˙
 */

// Low voltage Cuttoff
#define LOW_VOLTAGE_LIMIT_VOLTS 10.0f

// Max LV Bus current
#define LV_MAX_CURRENT_AMPS 30.0f
//
// TODO: We should verify that no channels draw less this when it is on
#define FUSE_BLOWN_MIN_CURRENT_AMPS 0.02

// Todo: verify these values from the schematic
#define ADC_TO_AMPS_DIVIDER 0.0002
#define ADC_TO_VOLTS_DIVIDER 0.207143

 // #define MOCK_ADC_READINGS

float readBusCurrent();
float readBusVoltage();
float readCurrent(PDU_ADC1_Channels_t channel);
void printRawADCVals();


#endif /* end of include guard: SENSORS_H */
