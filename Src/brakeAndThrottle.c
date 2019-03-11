#include "brakeAndThrottle.h"
#include "debug.h"

#if IS_BOARD_NUCLEO_F7
#define MOCK_ADC_READINGS
#endif

#define MIN_BRAKE_PRESSED_VAL_PERCENT 20
#define BRAKE_MSG_TIMEOUT_MS 100

#define TPS_TOLERANCE_PERCENT 10
#define TPS_MAX_WHILE_BRAKE_PRESSED_PERCENT 25
#define TPS_WHILE_BRAKE_PRESSED_RESET_PERCENT 5

#define THROTT_A_LOW (0xd44)
#define THROTT_B_LOW (0x5d2)

#define THROTT_A_HIGH (0xf08)
#define THROTT_B_HIGH (0x71f)

#define MAX_THROTTLE_DEADZONE (0x20)

uint32_t brakeThrottleSteeringADCVals[NUM_ADC_CHANNELS] = {0};

HAL_StatusTypeDef startADCConversions()
{
#ifndef MOCK_ADC_READINGS
    if (HAL_ADC_Start_DMA(&ADC_HANDLE, brakeThrottleSteeringADCVals, NUM_ADC_CHANNELS) != HAL_OK)
    {
        ERROR_PRINT("Failed to start ADC DMA conversions\n");
        Error_Handler();
        return HAL_ERROR;
    }
#else
    for (int i=0; i < NUM_ADC_CHANNELS; i++) {
        brakeThrottleSteeringADCVals[i] = 0;
    }
#endif

    return HAL_OK;
}

// Flag set if throttle and brake pressed at same time
bool throttleAndBrakePressedError = false;

float getBrakePositionPercent()
{
    return ((float)brakeThrottleSteeringADCVals[BRAKE_POS_INDEX]) / TPS_DIVISOR * TPS_MULTPLIER;
}

bool isBrakePressed()
{
    if (getBrakePositionPercent() > MIN_BRAKE_PRESSED_VAL_PERCENT) {
        return true;
    } else {
        return false;
    }
}

int map_range(int in, int low, int high, int low_out, int high_out) {
    if (in < low) {
        in = low;
    } else if (in > high) {
        in = high;
    }
    int in_range = high - low;
    int out_range = high_out - low_out;

    return (in - low) * out_range / in_range + low_out;
}

bool is_throttle1_in_range(uint32_t throttle) {
  return throttle <= THROTT_A_HIGH && throttle >= THROTT_A_LOW - MAX_THROTTLE_DEADZONE;
}

bool is_throttle2_in_range(uint32_t throttle) {
  return throttle <= THROTT_B_HIGH && throttle >= THROTT_B_LOW - MAX_THROTTLE_DEADZONE;
}

uint16_t calculate_throttle_percent1(uint16_t tps_value)
{
    return map_range(tps_value, THROTT_A_LOW, THROTT_A_HIGH,
      0, 100);
}

uint16_t calculate_throttle_percent2(uint16_t tps_value)
{
    return map_range(tps_value, THROTT_B_LOW, THROTT_B_HIGH,
      0, 100);
}

bool is_tps_within_tolerance(uint16_t throttle1_percent, uint16_t throttle2_percent)
{
    if (throttle1_percent == throttle2_percent
        || ((throttle1_percent > throttle2_percent) && ((throttle1_percent - throttle2_percent) < TPS_TOLERANCE_PERCENT))
        || ((throttle2_percent > throttle1_percent) && ((throttle2_percent - throttle1_percent) < TPS_TOLERANCE_PERCENT)))
    {
        return true;
    } else {
        return false;
    }
}


/*
 * This functions gets the current throttle value, performs the throttle and
 * brake plausibility checks, and returns the throttle value in throttle out
 * @return: true if throttle ok, false if not
 *
 */
ThrottleStatus_t getNewThrottle(float *throttleOut)
{
    uint32_t throttle1_percent, throttle2_percent;
    float throttle = 0;
    (*throttleOut) = 0;

    // Read both TPS sensors
    if (is_throttle1_in_range(brakeThrottleSteeringADCVals[THROTTLE_A_INDEX])
        && is_throttle2_in_range(brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]))
    {
        throttle1_percent = calculate_throttle_percent1(brakeThrottleSteeringADCVals[THROTTLE_A_INDEX]);
        throttle2_percent = calculate_throttle_percent2(brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]);
    } else {
      ERROR_PRINT("Throttle pot out of range: (A: %lu, B: %lu)\n", brakeThrottleSteeringADCVals[THROTTLE_A_INDEX], brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]);
      return THROTTLE_FAULT;
    }

    // Check if two throttle pots agree
    if(!is_tps_within_tolerance(throttle1_percent, throttle2_percent))
    {
        (*throttleOut) = 0;
        ERROR_PRINT("implausible pedal! %lu\r\n", throttle1_percent - throttle2_percent);
        return THROTTLE_FAULT;
    } else {
        throttle = (throttle1_percent + throttle2_percent) / 2;
    }

    // Both throttle and brake were pressed, check if still the case
    if (throttleAndBrakePressedError) {
        if (throttle < TPS_WHILE_BRAKE_PRESSED_RESET_PERCENT) {
            throttleAndBrakePressedError = false;
        } else {
            (*throttleOut) = 0;
            return THROTTLE_DISABLED;
        }
    }

    // check if both throttle and brake are pressed
    if (isBrakePressed() && throttle > TPS_MAX_WHILE_BRAKE_PRESSED_PERCENT) {
        (*throttleOut) = 0;
        throttleAndBrakePressedError = true;
        return THROTTLE_DISABLED;
    }

    // If we get here, all checks have passed, so can safely output throttle
    (*throttleOut) = throttle;

    return THROTTLE_OK;
}
