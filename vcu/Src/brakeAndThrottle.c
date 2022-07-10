#include "brakeAndThrottle.h"
#include "debug.h"
#include "motorController.h"
#include "vcu_F7_can.h"
#include "vcu_F7_dtc.h"

#if IS_BOARD_NUCLEO_F7
#define MOCK_ADC_READINGS
#endif

#define MIN_BRAKE_PRESSED_VAL_PERCENT 10
#define MIN_BRAKE_PRESSED_HARD_VAL_PERCENT 40
#define MAX_ZERO_THROTTLE_VAL_PERCENT 1

#define TPS_TOLERANCE_PERCENT 10
#define TPS_MAX_WHILE_BRAKE_PRESSED_PERCENT 25
#define TPS_WHILE_BRAKE_PRESSED_RESET_PERCENT 5

#define THROTT_A_LOW (1810)
#define THROTT_B_LOW (616)

#define THROTT_A_HIGH (2095)
#define THROTT_B_HIGH (939)

#define BRAKE_POS_LOW (1117)
#define BRAKE_POS_HIGH (1410)

#define STEERING_POT_LOW (1040)
#define STEERING_POT_CENTER (2122)
#define STEERING_SCALE_DIVIDER ((STEERING_POT_CENTER-STEERING_POT_LOW)/(90))
#define STEERING_POT_OFFSET (STEERING_POT_CENTER)

/*#define THROTT_A_LOW (0xd44)*/
/*#define THROTT_B_LOW (0x5d2)*/

/*#define THROTT_A_HIGH (0xf08)*/
/*#define THROTT_B_HIGH (0x71f)*/

#define MAX_THROTTLE_A_DEADZONE (200)
#define MAX_THROTTLE_B_DEADZONE (200)
/*#define MAX_THROTTLE_DEADZONE (0x20)*/

#define VCU_DATA_PUBLISH_TIME_MS 200


uint32_t brakeThrottleSteeringADCVals[NUM_ADC_CHANNELS] = {0};

HAL_StatusTypeDef startADCConversions()
{
#ifndef MOCK_ADC_READINGS
    /*DEBUG_PRINT("Starting adc readings for %d channels\n", NUM_ADC_CHANNELS);*/
    if (HAL_ADC_Start_DMA(&ADC_HANDLE, brakeThrottleSteeringADCVals, NUM_ADC_CHANNELS) != HAL_OK)
    {
        ERROR_PRINT("Failed to start ADC DMA conversions\n");
        Error_Handler();
        return HAL_ERROR;
    }
    if (HAL_TIM_Base_Start(&BRAKE_ADC_TIM_HANDLE) != HAL_OK) {
      ERROR_PRINT("Failed to start brake and throttle adc timer\n");
      Error_Handler();
      return HAL_ERROR;
    }
#else
    for (int i=0; i < NUM_ADC_CHANNELS; i++) {
        if (i == BRAKE_PRES_INDEX) {
            brakeThrottleSteeringADCVals[i] = 95 * BRAKE_PRESSURE_DIVIDER / BRAKE_PRESSURE_MULTIPLIER;
        } else {
            brakeThrottleSteeringADCVals[i] = 0;
        }

        brakeThrottleSteeringADCVals[THROTTLE_A_INDEX] = calculate_throttle_adc_from_percent1(0);
        brakeThrottleSteeringADCVals[THROTTLE_B_INDEX] = calculate_throttle_adc_from_percent2(0);
    }
#endif

    return HAL_OK;
}

// Flag set if throttle and brake pressed at same time
bool throttleAndBrakePressedError = false;

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

float getBrakePositionPercent()
{
    return map_range(brakeThrottleSteeringADCVals[BRAKE_POS_INDEX],
                           BRAKE_POS_LOW, BRAKE_POS_HIGH, 0, 100);
}

bool is_throttle1_in_range(uint32_t throttle) {
  return throttle <= THROTT_A_HIGH+MAX_THROTTLE_A_DEADZONE && throttle >= THROTT_A_LOW-MAX_THROTTLE_A_DEADZONE;
}

bool is_throttle2_in_range(uint32_t throttle) {
  return throttle <= THROTT_B_HIGH+MAX_THROTTLE_B_DEADZONE && throttle >= THROTT_B_LOW - MAX_THROTTLE_B_DEADZONE;
}

uint16_t calculate_throttle_percent1(uint16_t tps_value)
{
    // Throttle A is inverted
    return 100 - map_range(tps_value, THROTT_A_LOW, THROTT_A_HIGH,
      0, 100);
}

uint16_t calculate_throttle_percent2(uint16_t tps_value)
{
    return map_range(tps_value, THROTT_B_LOW, THROTT_B_HIGH,
      0, 100);
}

// These are for testing
uint16_t calculate_throttle_adc_from_percent1(uint16_t percent)
{
  return map_range(percent, 0, 100, THROTT_A_LOW, THROTT_A_HIGH);
}
uint16_t calculate_throttle_adc_from_percent2(uint16_t percent)
{
  return map_range(percent, 0, 100, THROTT_B_LOW, THROTT_B_HIGH);
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


// Get the throttle posiition as a percent
// @ret False if implausibility, true otherwise
bool getThrottlePositionPercent(float *throttleOut)
{
    uint32_t throttle1_percent, throttle2_percent;
    float throttle;
    (*throttleOut) = 0;

    // Read both TPS sensors
    if (is_throttle1_in_range(brakeThrottleSteeringADCVals[THROTTLE_A_INDEX])
        && is_throttle2_in_range(brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]))
    {
        throttle1_percent = calculate_throttle_percent1(brakeThrottleSteeringADCVals[THROTTLE_A_INDEX]);
        throttle2_percent = calculate_throttle_percent2(brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]);
    } else {
      ERROR_PRINT("Throttle pot out of range: (A: %lu, B: %lu)\n", brakeThrottleSteeringADCVals[THROTTLE_A_INDEX], brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]);
      return false;
    }

    // Check if two throttle pots agree
    if(!is_tps_within_tolerance(throttle1_percent, throttle2_percent))
    {
        (*throttleOut) = 0;
        ERROR_PRINT("implausible pedal! difference: %ld %%\r\n", throttle1_percent - throttle2_percent);
        DEBUG_PRINT("Throttle A: %lu, Throttle B: %lu\n", brakeThrottleSteeringADCVals[THROTTLE_A_INDEX], brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]);
        return false;
    } else {
        /*DEBUG_PRINT("t1 %ld, t2 %ld\n", throttle1_percent, throttle2_percent);*/
        throttle = (throttle1_percent + throttle2_percent) / 2;
    }

    *throttleOut = throttle;
    return true;
}

/*
 * This functions gets the current throttle value, performs the throttle and
 * brake plausibility checks, and returns the throttle value in throttle out
 * @return: true if throttle ok, false if not
 *
 */
ThrottleStatus_t getNewThrottle(float *throttleOut)
{
    float throttle = 0;
    (*throttleOut) = 0;

    if (!getThrottlePositionPercent(&throttle)) {
      DEBUG_PRINT("Throttle error\n");
        return THROTTLE_FAULT;
    }

    // Both throttle and brake were pressed, check if still the case
    if (throttleAndBrakePressedError) {
        if (throttle < TPS_WHILE_BRAKE_PRESSED_RESET_PERCENT) {
            throttleAndBrakePressedError = false;
            sendDTC_WARNING_BrakeWhileThrottleError_Enabled();
        } else {
            (*throttleOut) = 0;
            DEBUG_PRINT("Throttle disabled, brake was pressed and throttle still not zero\n");
            return THROTTLE_DISABLED;
        }
    }

    // check if both throttle and brake are pressed
    if (isBrakePressedHard() && throttle > TPS_MAX_WHILE_BRAKE_PRESSED_PERCENT) {
        (*throttleOut) = 0;
        throttleAndBrakePressedError = true;
        sendDTC_WARNING_BrakeWhileThrottleError_Disabled();
        DEBUG_PRINT("Throttle disabled, brakePressed\n");
        return THROTTLE_DISABLED;
    }

    // If we get here, all checks have passed, so can safely output throttle
    (*throttleOut) = throttle;

    return THROTTLE_OK;
}

/* Public Functions */
HAL_StatusTypeDef outputThrottle() {
    float throttle;

    ThrottleStatus_t rc = getNewThrottle(&throttle);

    if (rc == THROTTLE_FAULT) {
        return HAL_ERROR;
    } else if (rc == THROTTLE_DISABLED) {
        DEBUG_PRINT("Throttle disabled due brake pressed\n");
    }

    static uint64_t count = 0;
    count++;
    if (count % 20 == 0) {
      DEBUG_PRINT("Setting MC throttles to %f\n", throttle);
    }
    sendThrottleValueToMCs(throttle, getSteeringAngle());

    return HAL_OK;
}

bool isBrakePressedHard()
{
    if (getBrakePositionPercent() > MIN_BRAKE_PRESSED_HARD_VAL_PERCENT) {
        return true;
    } else {
        return false;
    }
}
bool isBrakePressed()
{
    /*DEBUG_PRINT("Brake %f\n", getBrakePositionPercent());*/

    if (getBrakePositionPercent() > MIN_BRAKE_PRESSED_VAL_PERCENT) {
        return true;
    } else {
        return false;
    }
}

bool throttleIsZero()
{
    float throttle;

    if (!getThrottlePositionPercent(&throttle)) {
      return false;
    } else if (throttle < MAX_ZERO_THROTTLE_VAL_PERCENT) {
      return true;
    } else {
      return false;
    }
}

bool checkBPSState() {
  // TODO: Monitor BPS for failures
  return true;
}

int getBrakePressure() {
  return brakeThrottleSteeringADCVals[BRAKE_PRES_INDEX] * BRAKE_PRESSURE_MULTIPLIER / BRAKE_PRESSURE_DIVIDER;
}

int getSteeringAngle() {
  int steeringPotVal = brakeThrottleSteeringADCVals[STEERING_INDEX];

  return -1 *(steeringPotVal - STEERING_POT_OFFSET) / STEERING_SCALE_DIVIDER;
}


HAL_StatusTypeDef brakeAndThrottleStart()
{
    if (startADCConversions() != HAL_OK)
    {
        ERROR_PRINT("Failed to start brake and throttle ADC conversions\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

void canPublishTask(void *pvParameters)
{
  float throttle;
  while (1) {
    // Delay to allow first ADC readings to come in
    vTaskDelay(500);

    // Update value to be sent over can
    getThrottlePositionPercent(&throttle);
    ThrottlePercent = throttle;
    brakePressure = getBrakePressure();
    SteeringAngle = getSteeringAngle();
    BrakePercent = getBrakePositionPercent();

    if (sendCAN_VCU_Data() != HAL_OK) {
      ERROR_PRINT("Failed to send vcu can data\n");
    }
    vTaskDelay(pdMS_TO_TICKS(VCU_DATA_PUBLISH_TIME_MS));
  }
}
