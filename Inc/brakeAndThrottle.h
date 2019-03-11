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
#define TPS_DIVISOR 4096
#define TPS_MAX 4096
/* End of CLI mock testing stuff */

typedef enum ThrottleStatus_t {
    THROTTLE_OK, // Throttle is OK
    THROTTLE_DISABLED, // Throttle implausibility occured, but don't need to enter fault state yet, the value returned for throttle value will be zero
    THROTTLE_FAULT // Throttle fault occured, need to enter fault state
} ThrottleStatus_t;

bool isBrakePressed();

#endif /* end of include guard: BRAKEANDTHROTTLE_H */
