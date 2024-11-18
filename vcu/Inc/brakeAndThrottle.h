#ifndef BRAKEANDTHROTTLE_H

#define BRAKEANDTHROTTLE_H

#include "bsp.h"
#include "FreeRTOS.h"

#define MIN_BRAKE_PRESSED_VAL_PERCENT 15
#define APPS_BRAKE_PLAUSIBILITY_THRESHOLD 40  // set experimentally based on driver feedback
#define MAX_ZERO_THROTTLE_VAL_PERCENT 2

#define TPS_TOLERANCE_PERCENT 15 // Should be 10 but pots are noisy
#define TPS_MAX_WHILE_BRAKE_PRESSED_PERCENT 25
#define TPS_WHILE_BRAKE_PRESSED_RESET_PERCENT 5

#define THROTT_A_LOW (2272)
#define THROTT_B_LOW (2160)

#define THROTT_A_HIGH (2500)
#define THROTT_B_HIGH (2405)

#define BRAKE_POS_LOW (1080)
#define BRAKE_POS_HIGH (1180)


#define STEERING_POT_LOW (1) //Pot value when the wheel is all the way to the left
#define STEERING_POT_HIGH (4095) //Pot value when the wheel is all the way to the right

#define STEERING_POT_CENTER (((STEERING_POT_HIGH-STEERING_POT_LOW)/2) + STEERING_POT_LOW) //The pot value while the wheel is neutral
#define STEERING_SCALE_DIVIDER (STEERING_POT_CENTER/(100)) //Scale the pot value to range (-100,100) 
#define STEERING_POT_OFFSET (STEERING_POT_CENTER)

/*#define THROTT_A_LOW (0xd44)*/
/*#define THROTT_B_LOW (0x5d2)*/

/*#define THROTT_A_HIGH (0xf08)*/
/*#define THROTT_B_HIGH (0x71f)*/

#define MAX_THROTTLE_A_DEADZONE (200)
#define MAX_THROTTLE_B_DEADZONE (200)
/*#define MAX_THROTTLE_DEADZONE (0x20)*/

#define THROTTLE_POLLING_TASK_ID 4
#define THROTTLE_POLLING_FLAG_BIT (0)
#define VCU_DATA_PUBLISH_TIME_MS 50
#define THROTTLE_POLLING_TASK_PERIOD_MS 50

typedef enum ADC_Indices_t {
    THROTTLE_A_INDEX = 0,
    THROTTLE_B_INDEX,
    BRAKE_POS_INDEX,
    STEERING_INDEX,
    BRAKE_PRES_INDEX,
    NUM_ADC_CHANNELS
} ADC_Indices_t;

#define TPS_MULTPLIER 100
#define TPS_DIVISOR 4095
#define TPS_MAX 4095

#define BRAKE_POSITION_DIVIDER 4095
#define BRAKE_POSITION_MULTIPLIER 100

////brake pressure new stuff, untested
#define BRAKE_PRES_ADC_LOW (409) //at 500mV (the sensor minumum)
#define BRAKE_PRES_ADC_HIGH (3686) //at 4500mV (the sensor maximum)
#define BRAKE_PRES_PSI_LOW (0) //this is a 0-2000PSI sensor
#define BRAKE_PRES_PSI_HIGH (2000)
#define BRAKE_PRES_DEADZONE (100)
////

#define STEERING_DIVIDER 4095
#define STEERING_MULTIPLIER 100
#define STEERING_CENTRE_OFFSET_PERCENT 50

/* End of CLI mock testing stuff */

typedef enum ThrottleStatus_t {
    THROTTLE_OK, // Throttle is OK
    THROTTLE_DISABLED, // Throttle disabled due to brake being pressed, but don't need to enter fault state, the value returned for throttle value will be zero
    THROTTLE_FAULT // Throttle fault occured, need to enter fault state
} ThrottleStatus_t;

bool isBrakePressed();
bool throttleIsZero();
void throttlePollingTask(void);
bool checkBPSState();
int getBrakePressure();
HAL_StatusTypeDef brakeAndThrottleStart();
int getSteeringAngle();
float getBrakePositionPercent();

// For testing
uint16_t calculate_throttle_adc_from_percent1(uint16_t percent);
uint16_t calculate_throttle_adc_from_percent2(uint16_t percent);
float calculate_throttle_percent1(uint16_t tps_value);
float calculate_throttle_percent2(uint16_t tps_value);
ThrottleStatus_t getNewThrottle(float *throttleOut);

#endif /* end of include guard: BRAKEANDTHROTTLE_H */
