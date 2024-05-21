#include "brakeAndThrottle.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "watchdog.h"
#include "motorController.h"
#include "vcu_F7_can.h"
#include "vcu_F7_dtc.h"
#include "drive_by_wire.h"
#include "state_machine.h"
#include "drive_by_wire.h"
#include "canReceive.h"
#include "mathUtils.h"

#if IS_BOARD_NUCLEO_F7
#define MOCK_ADC_READINGS
#endif

uint32_t brakeThrottleSteeringADCVals[NUM_ADC_CHANNELS] = {0};
static float throttlePercentReading = 0.0f;

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
    return map_range(   brakeThrottleSteeringADCVals[BRAKE_POS_INDEX],
                        BRAKE_POS_LOW, BRAKE_POS_HIGH, 0, 100);
}

bool is_throttle1_in_range(uint32_t throttle) {
  return throttle <= THROTT_A_HIGH+MAX_THROTTLE_A_DEADZONE && throttle >= THROTT_A_LOW-MAX_THROTTLE_A_DEADZONE;
}

bool is_throttle2_in_range(uint32_t throttle) {
  return throttle <= THROTT_B_HIGH+MAX_THROTTLE_B_DEADZONE && throttle >= THROTT_B_LOW - MAX_THROTTLE_B_DEADZONE;
}

float calculate_throttle_percent1(uint16_t tps_value)
{
    // Throttle A is inverted
    return 100.0f - map_range_float((float)tps_value, THROTT_A_LOW, THROTT_A_HIGH,
      0, 100);
}

float calculate_throttle_percent2(uint16_t tps_value)
{
    return map_range_float((float)tps_value, THROTT_B_LOW, THROTT_B_HIGH,
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

bool is_tps_within_tolerance(float throttle1_percent, float throttle2_percent)
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

// Get the throttle position as a percent
// @ret False if implausibility, true otherwise
bool getThrottlePositionPercent(float *throttleOut)
{
    float throttle1_percent, throttle2_percent;
    float throttle;
    (*throttleOut) = 0;


    ThrottleAReading = brakeThrottleSteeringADCVals[THROTTLE_A_INDEX];
    ThrottleBReading = brakeThrottleSteeringADCVals[THROTTLE_B_INDEX];
    BrakeReading = brakeThrottleSteeringADCVals[BRAKE_POS_INDEX];
    sendCAN_VCU_ADCReadings();

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
        ERROR_PRINT("implausible pedal! difference: %f %%\r\n", throttle1_percent - throttle2_percent);
        DEBUG_PRINT("Throttle A: %lu, Throttle B: %lu\n", brakeThrottleSteeringADCVals[THROTTLE_A_INDEX], brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]);
        return false;
    } else {
        /*DEBUG_PRINT("t1 %ld, t2 %ld\n", throttle1_percent, throttle2_percent);*/
        throttle = 100-((throttle1_percent + throttle2_percent) / 2);
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

// Full left turn angle: -100 degrees
// Full right turn angle: 100 degrees
int getSteeringAngle() {
  int steeringPotVal = brakeThrottleSteeringADCVals[STEERING_INDEX];
  return (steeringPotVal-STEERING_POT_OFFSET) / STEERING_SCALE_DIVIDER;
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
    // Delay to allow first ADC readings to come in
    vTaskDelay(500);
    while (1) {

        // Update value to be sent over can
        ThrottlePercent = throttlePercentReading;
        brakePressure = getBrakePressure();
        SteeringAngle = getSteeringAngle();
        BrakePercent = getBrakePositionPercent();
        BrakeWhileThrottle = throttleAndBrakePressedError;

        if (sendCAN_VCU_Data() != HAL_OK) {
            ERROR_PRINT("Failed to send vcu can data\n");
        }
        vTaskDelay(pdMS_TO_TICKS(VCU_DATA_PUBLISH_TIME_MS));
    }
}

HAL_StatusTypeDef pollThrottle(void) {
    ThrottleStatus_t rc = getNewThrottle(&throttlePercentReading);

    if (rc != THROTTLE_OK)
    {
        if (rc == THROTTLE_FAULT) {
            sendDTC_WARNING_Throttle_Failure(0);
            DEBUG_PRINT("Throttle value out of range\n");
        } else if (rc == THROTTLE_DISABLED) {
            sendDTC_WARNING_Throttle_Failure(1);
            DEBUG_PRINT("Throttle disabled as brake pressed\n");
            return HAL_OK;
        } else {
            sendDTC_WARNING_Throttle_Failure(2);
            DEBUG_PRINT("Unknown throttle error\r\n");
        }
        return HAL_ERROR;
    }

    if(isLockoutDisabled()) {
        // Send torque request to MC
        return requestTorqueFromMC(throttlePercentReading);                
    } else {
        // Send lockout release to MC
        return sendLockoutReleaseToMC();
    }
}

void throttlePollingTask(void) 
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    if (registerTaskToWatch(THROTTLE_POLLING_TASK_ID, 2*pdMS_TO_TICKS(THROTTLE_POLLING_TASK_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("ERROR: Failed to init throttle polling task, suspending throttle polling task\n");
        while(1);
    }

    while (1)
    {
        // Once EM Enabled, start polling throttle
        if (fsmGetState(&fsmHandle) == STATE_EM_Enable)
        {
            // Check motor controller status
            bool inverterFault = getInverterVSMState() == INV_VSM_State_FAULT_STATE;
            if (inverterFault) {   
                // DTC sent in state machine transition function
                uint64_t faults = getInverterFaultCode();
                DEBUG_PRINT("Inverter fault %lu\n", (uint32_t) faults);
            }

            // Poll throttle
            if (pollThrottle() != HAL_OK) {
                ERROR_PRINT("ERROR: Failed to request torque from MC\n");
                fsmSendEventUrgent(&fsmHandle, EV_Throttle_Failure, portMAX_DELAY);
            }
        }
        else
        {
            // EM disabled
            throttlePercentReading = 0;
        }
            
        watchdogTaskCheckIn(THROTTLE_POLLING_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(THROTTLE_POLLING_TASK_PERIOD_MS));
    }
}
