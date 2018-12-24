#ifndef SENSORS_H

#define SENSORS_H

/*
 * !!IMPORTANT!!
 * These are numbered to match which ADC channel they are connected to, don't
 * change the order or add channels unless you know what you're doing!!
 */
typedef enum PDU_Channels_t {
    Fan_Right_Channel, // Channel 2
    DCU_Channel, // Channel 3
    MC_Left_Channel, // Channel 4
    Pump_Left_Channel, // Channel 5
    Fan_Left_Channel, // Channel 6
    VCU_Channel, // Channel 7
    Telemetery_Channel, // Channel 8
    Fan_Batt_Channel, // Channel 9
    LV_Current, // Channel 10
    LV_Voltage, // Channel 11
    MC_Right_Channel, // Channel 12
    Pump_Right_Channel, // Channel 13
    BMU_Channel, // Channel 14
    WSB_Channel, // Channel 15
    NUM_PDU_CHANNELS
} PDU_Channels_t;

#define SENSOR_READ_PERIOD_MS 500

/*
 * Sensor Valid Ranges
 */

// Low voltage Cuttoff
#define LOW_VOLTAGE_LIMIT_VOLTS 10.0f

// Max LV Bus current
#define LV_MAX_CURRENT_AMPS 50.0f

#endif /* end of include guard: SENSORS_H */
