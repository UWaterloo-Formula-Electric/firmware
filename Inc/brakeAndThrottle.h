#ifndef BRAKEANDTHROTTLE_H

#define BRAKEANDTHROTTLE_H

#include "bsp.h"
#include "freertos.h"

/* Only declared public for use in CLI for mock testing */
typedef enum ADC_Indices_t {
    THROTTLE_A_INDEX = 0,
    THROTTLE_B_INDEX,
    BRAKE_POS_INDEX,
    BRAKE_PRES_INDEX,
    STEERING_INDEX,
    NUM_ADC_CHANNELS
} ADC_Indices_t;

#define TPS_MULTPLIER 100
#define TPS_DIVISOR 4095
#define TPS_MAX 4095

#define BRAKE_PRESSURE_DIVIDER 4095
#define BRAKE_PRESSURE_MULTIPLIER 100
/* End of CLI mock testing stuff */

typedef enum ThrottleStatus_t {
    THROTTLE_OK, // Throttle is OK
    THROTTLE_DISABLED, // Throttle disabled due to brake being pressed, but don't need to enter fault state, the value returned for throttle value will be zero
    THROTTLE_FAULT // Throttle fault occured, need to enter fault state
} ThrottleStatus_t;

bool isBrakePressed();
HAL_StatusTypeDef outputThrottle();
bool throttleIsZero();
bool checkBPSState();
int getBrakePressurePercent();
HAL_StatusTypeDef brakeAndThrottleStart();

// For testing
uint16_t calculate_throttle_adc_from_percent1(uint16_t percent);
uint16_t calculate_throttle_adc_from_percent2(uint16_t percent);

#endif /* end of include guard: BRAKEANDTHROTTLE_H */
