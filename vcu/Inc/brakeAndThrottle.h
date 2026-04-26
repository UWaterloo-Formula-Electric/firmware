#ifndef BRAKEANDTHROTTLE_H

#define BRAKEANDTHROTTLE_H

#include "FreeRTOS.h"
#include "bsp.h"

#define ADC_RESOLUTION_BITS 12
#define ADC_MAX_COUNT 4095
#define ADC_MAX_VALUE ((float)ADC_MAX_COUNT)
#define ADC_12_BIT_2_12_BIT(x) ((x) >> (ADC_RESOLUTION_BITS - ADC_RESOLUTION_BITS))

#define PERCENT_MIN 0.0f
#define PERCENT_MAX 100.0f
#define PERCENT_MIN_INT 0
#define PERCENT_MAX_INT 100
#define TPS_SENSOR_COUNT 2.0f
#define THROTTLE_FAILURE_DTC_DETAIL 0

#define MIN_BRAKE_PRESSED_VAL_PERCENT 15
#define APPS_BRAKE_PLAUSIBILITY_THRESHOLD 40  // set experimentally based on driver feedback
#define MAX_ZERO_THROTTLE_VAL_PERCENT 2

// Brake pressure above which 100% braking force is assumed — tune experimentally
#define BRAKE_PRESSURE_100_PERCENT_PSI 1000.0f
#define BRAKE_PRESSURE_SENSOR_MIN_V 0.5f
#define BRAKE_PRESSURE_SENSOR_MAX_V 4.5f
#define BRAKE_PRESSURE_SENSOR_MIN_PSI 0.0f
#define BRAKE_PRESSURE_SENSOR_MAX_PSI 2500.0f
#define BRAKE_PRESSURE_ADC_REF_V 3.3f
#define BRAKE_PRESSURE_VOLTAGE_DIVIDER_SCALE 1.5f
#define MOCK_BRAKE_PRESSURE_PERCENT 95
// Minimum brake position % before the pos-vs-pressure cross-check is active
#define BRAKE_POS_IMPLAUSIBILITY_MIN_PERCENT 20.0f
// If brake position % exceeds brake pressure % by this much, flag an implausibility
#define BRAKE_IMPLAUSIBILITY_DIFF_PERCENT 20.0f

#define TPS_TOLERANCE_PERCENT 15  // Should be 10 but pots are noisy
#define TPS_MAX_WHILE_BRAKE_PRESSED_PERCENT 25
#define TPS_WHILE_BRAKE_PRESSED_RESET_PERCENT 5

#define THROTT_A_LOW (355)
#define THROTT_B_LOW (2115)

#define THROTT_A_HIGH (2010)
#define THROTT_B_HIGH (3765)

#define BRAKE_POS_LOW (165)
#define BRAKE_POS_HIGH (235)

#define STEERING_POT_LOW (1)      // Pot value when the wheel is all the way to the left
#define STEERING_POT_HIGH (ADC_MAX_COUNT)  // Pot value when the wheel is all the way to the right

#define STEERING_POT_CENTER (((STEERING_POT_HIGH - STEERING_POT_LOW) / 2) + STEERING_POT_LOW)  // The pot value while the wheel is neutral
#define STEERING_SCALE_DIVIDER (STEERING_POT_CENTER / PERCENT_MAX_INT)                         // Scale the pot value to range (-100,100)
#define STEERING_POT_OFFSET (STEERING_POT_CENTER)

#define MAX_THROTTLE_A_DEADZONE (30)
#define MAX_THROTTLE_B_DEADZONE (30)

#define INV_COMMAND_TASK_ID 4
#define INV_COMMAND_FLAG_BIT (0)
#define VCU_DATA_PUBLISH_TIME_MS 10
#define INV_COMMAND_TASK_PERIOD_MS 4
#define VCU_DATA_STARTUP_DELAY_MS 500
#define INV_COMMAND_WATCHDOG_PERIOD_MULTIPLIER 2

#define MEDIAN_FILTER_MS 40
#define NUM_MEDIAN_FILTER_SAMPLES (MEDIAN_FILTER_MS / INV_COMMAND_TASK_PERIOD_MS)  // Number of samples to take for median filter

typedef enum ADC_Indices_t {
    THROTTLE_A_INDEX = 0,
    THROTTLE_B_INDEX,
    BRAKE_POS_INDEX,
    BRAKE_PRES_INDEX,
    STEERING_INDEX,
    NUM_ADC_CHANNELS
} ADC_Indices_t;

#define TPS_MULTPLIER PERCENT_MAX_INT
#define TPS_DIVISOR ADC_MAX_COUNT
#define TPS_MAX ADC_MAX_COUNT

#define BRAKE_POSITION_DIVIDER ADC_MAX_COUNT
#define BRAKE_POSITION_MULTIPLIER PERCENT_MAX_INT

#define BRAKE_PRESSURE_DIVIDER ADC_MAX_COUNT
#define BRAKE_PRESSURE_MULTIPLIER PERCENT_MAX_INT

#define STEERING_DIVIDER ADC_MAX_COUNT
#define STEERING_MULTIPLIER PERCENT_MAX_INT
#define STEERING_CENTRE_OFFSET_PERCENT 50


bool isRegenEnabled();
void toggleRegen();
void disableRegen();

typedef enum ThrottleStatus_t {
    THROTTLE_OK,        // Throttle is OK
    THROTTLE_DISABLED,  // Throttle disabled due to brake being pressed, but don't need to enter fault state, the value returned for throttle value will be zero
    THROTTLE_FAULT      // Throttle fault occured, need to enter fault state
} ThrottleStatus_t;

bool isBrakePressed();
bool throttleIsZero();
bool checkBPSState();
int getBrakePressure();
float getBrakePressurePercent();
HAL_StatusTypeDef brakeAndThrottleStart();
int getSteeringAngle();
float getBrakePositionPercent();

float getThrottleAFiltered();
float getThrottleBFiltered();

// For testing
uint16_t calculate_throttle_adc_from_percent1(uint16_t percent);
uint16_t calculate_throttle_adc_from_percent2(uint16_t percent);
float calculate_throttle_percent1(uint16_t tps_value);
float calculate_throttle_percent2(uint16_t tps_value);
ThrottleStatus_t getNewThrottle(float *throttleOut);

#endif /* end of include guard: BRAKEANDTHROTTLE_H */
