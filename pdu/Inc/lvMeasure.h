#ifndef LV_MEASURE_H
#define LV_MEASURE_H

/*
 * !!IMPORTANT!!
 * These are numbered to match which ADC channel they are connected to, don't
 * change the order or add channels unless you know what you're doing!!
 */

/* This is enum is for all the PDU output channels */

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

typedef struct {
    float I_Main_Channel_A;
    float V_Main_Channel_V;
    float V_Lipo_Channel_V;
    float I_DCDC_Channel_A;
    float V_DCDC_Channel_V;
    float Lipo_Therm_Channel_C;
} busMeas_t;

// This union allows us to print out the data easily
typedef union {
    busMeas_t meas_s;
    float busMeas_a[NUM_ADC1_CHANNELS];
} busMeas_u;

extern busMeas_u busResults;
extern volatile uint32_t ADC1_Buffer[NUM_ADC1_CHANNELS];
extern const char *diagnosticChannelNames[NUM_ADC1_CHANNELS];

#define SENSOR_READ_PERIOD_MS 500
#define PDU_CURRENT_PUBLISH_PERIOD_MS 1000
#define PDU_POWER_STATES_PUBLISH_PERIOD_MS 5000
#define PDU_FUSE_STATUS_PUBLISH_PERIOD_MS 10000
#define PDU_PUBLISH_PERIOD_MS 1000

// Low voltage Cuttoff
#define LOW_VOLTAGE_LIMIT_VOLTS 10.0f

// Max LV Bus current
#define LV_MAX_CURRENT_AMPS 30.0f           // Fuse is set 30A

// TODO: We should verify that no channels draw less this when it is on
#define FUSE_BLOWN_MIN_CURRENT_AMPS 0.02

// TODO: verify this. What's the current draw at LV?
#define MIN_BUS_CURRENT 1.0f

// Todo: verify these values from the schematic
#define ADC1_TO_VOLTS_DIVIDER   257.1084024         // 1 / ((3.3/4096.0) / (0.207143))
#define ADC1_TO_AMPS_DIVIDER    49.64848485         // 1 / ((3.3/4096.0)/ (0.0002 * 200)) // TODO: will be changed in REV2

 // #define MOCK_ADC_READINGS

float readCurrent(PDU_ADC1_Channels_t channel);
float readVoltage(PDU_ADC1_Channels_t channel);

void printRawADCVals(void);

#endif /* end of include guard: LV_MEASURE_H */
