#ifndef SENSE_H

#define SENSE_H

#define BRAKE_ADC_DIVIDER 1

// Only used by CLI
typedef enum {
    BRAKE_HALL_ADC_CHANNEL_HALL = 0U,
    BRAKE_HALL_ADC_CHANNEL_BRAKE,
    BRAKE_HALL_ADC_CHANNEL_NUM,
} BrakeHallAdc_Channel_E;

#endif /* end of include guard: SENSE_H */
