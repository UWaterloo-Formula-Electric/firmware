/**
 *******************************************************************************
 * @file    brakeAndThrottle.c
 * @author	Richard
 * @date    Dec 2024
 * @brief   Obtain readings for brake pressure, brake position, steering and 
 *          throttle position
 *
 ******************************************************************************
 */

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
#include "wheelConstants.h"
#include "mathUtils.h"

#if IS_BOARD_NUCLEO_F7
#define MOCK_ADC_READINGS
#endif

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/


bool appsBrakePedalPlausibilityCheckFail(float throttle);

uint32_t brakeThrottleSteeringADCVals[NUM_ADC_CHANNELS] = {0};
static float throttlePercentReading = 0.0f;

/*********************************************************************************************************************/
/*-----------------------------------------------------Helpers-------------------------------------------------------*/
/*********************************************************************************************************************/
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
    if (HAL_TIM_Base_Start(&ADC_TIM_HANDLE) != HAL_OK) {
      ERROR_PRINT("Failed to start brake and throttle adc timer\n");
      Error_Handler();
      return HAL_ERROR;
    }
#else
    for (int i=0; i < NUM_ADC_CHANNELS; i++) {
        if (i == BRAKE_PRES_INDEX) {
            brakeThrottleSteeringADCVals[i] = MOCK_BRAKE_PRESSURE_PERCENT * BRAKE_PRESSURE_DIVIDER / BRAKE_PRESSURE_MULTIPLIER;
        } else {
            brakeThrottleSteeringADCVals[i] = 0;
        }

        brakeThrottleSteeringADCVals[THROTTLE_A_INDEX] = calculate_throttle_adc_from_percent1(PERCENT_MIN_INT);
        brakeThrottleSteeringADCVals[THROTTLE_B_INDEX] = calculate_throttle_adc_from_percent2(PERCENT_MIN_INT);
    }
#endif

    return HAL_OK;
}

static float getBrakePosFiltered()
{
    static float brakePosReadings[NUM_MEDIAN_FILTER_SAMPLES] = {0};
    static size_t brakePosIndex = 0;
    brakePosReadings[brakePosIndex] = (float)brakeThrottleSteeringADCVals[BRAKE_POS_INDEX];
    brakePosIndex = (brakePosIndex + 1) % NUM_MEDIAN_FILTER_SAMPLES;
    return get_median(brakePosReadings, NUM_MEDIAN_FILTER_SAMPLES);
}

static float getBrakePresFiltered()
{
    static float brakePresReadings[NUM_MEDIAN_FILTER_SAMPLES] = {0};
    static size_t brakePresIndex = 0;
    brakePresReadings[brakePresIndex] = (float)brakeThrottleSteeringADCVals[BRAKE_PRES_INDEX];
    brakePresIndex = (brakePresIndex + 1) % NUM_MEDIAN_FILTER_SAMPLES;
    return get_median(brakePresReadings, NUM_MEDIAN_FILTER_SAMPLES);
}

static bool isBrakePositionRawInRange(float brakePos)
{
    return brakePos >= BRAKE_POS_LOW && brakePos <= BRAKE_POS_HIGH;
}

static float getBrakePotentiometerPercentFromRaw(float brakePos)
{
    return map_range_float(brakePos, BRAKE_POS_LOW, BRAKE_POS_HIGH, PERCENT_MIN, PERCENT_MAX);
}

float getBrakePotentiometerPercent()
{
    return getBrakePotentiometerPercentFromRaw(getBrakePosFiltered());
}

static bool brakePositionAndPressureAgree(float posPercent, float presPercent)
{
    float diff = posPercent - presPercent;
    if (diff < 0) {
        diff = -diff;
    }
    return diff <= BRAKE_POSITION_PRESSURE_TOLERANCE_PERCENT;
}

// Brake position is inverter (pressing brake decreases Pot value).
float getBrakePositionPercent()
{
    float posPercent = getBrakePotentiometerPercent();
    float presPercent = getBrakePressurePercent();
    if (brakePositionAndPressureAgree(posPercent, presPercent)) {
        return 100 - (posPercent + presPercent) / BRAKE_SENSOR_COUNT;
    }

    return 100 - max(posPercent, presPercent);
}

bool is_throttle1_in_range(uint32_t throttle) {
  return throttle <= THROTT_A_HIGH+MAX_THROTTLE_A_DEADZONE && throttle >= THROTT_A_LOW-MAX_THROTTLE_A_DEADZONE;
}

bool is_throttle2_in_range(uint32_t throttle) {
  return throttle <= THROTT_B_HIGH+MAX_THROTTLE_B_DEADZONE && throttle >= THROTT_B_LOW-MAX_THROTTLE_B_DEADZONE;
}

float calculate_throttle_percent1(uint16_t tps_value)
{
    // Throttle A is inverted
    return PERCENT_MAX - map_range_float((float)tps_value, THROTT_A_LOW, THROTT_A_HIGH,
      PERCENT_MIN, PERCENT_MAX);
}

float calculate_throttle_percent2(uint16_t tps_value)
{
    return map_range_float((float)tps_value, THROTT_B_LOW, THROTT_B_HIGH,
      PERCENT_MIN, PERCENT_MAX);
}

// These are for testing
uint16_t calculate_throttle_adc_from_percent1(uint16_t percent)
{
  return map_range(percent, PERCENT_MIN_INT, PERCENT_MAX_INT, THROTT_A_LOW, THROTT_A_HIGH);
}
uint16_t calculate_throttle_adc_from_percent2(uint16_t percent)
{
  return map_range(percent, PERCENT_MIN_INT, PERCENT_MAX_INT, THROTT_B_LOW, THROTT_B_HIGH);
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


float getThrottleAFiltered(){
    static float throttleAReadings[NUM_MEDIAN_FILTER_SAMPLES] = {0};
    static size_t throttleAIndex = 0;
    // Store the current reading in the array
    throttleAReadings[throttleAIndex] = ADC_12_BIT_2_12_BIT(brakeThrottleSteeringADCVals[THROTTLE_A_INDEX]);
    throttleAIndex = (throttleAIndex + 1) % NUM_MEDIAN_FILTER_SAMPLES;
    return get_median(throttleAReadings, NUM_MEDIAN_FILTER_SAMPLES);
}

float getThrottleBFiltered()
{
    static float throttleBReadings[NUM_MEDIAN_FILTER_SAMPLES] = {0};
    static size_t throttleBIndex = 0;
    // Store the current reading in the array
    throttleBReadings[throttleBIndex] = ADC_12_BIT_2_12_BIT(brakeThrottleSteeringADCVals[THROTTLE_B_INDEX]);
    throttleBIndex = (throttleBIndex + 1) % NUM_MEDIAN_FILTER_SAMPLES;
    return get_median(throttleBReadings, NUM_MEDIAN_FILTER_SAMPLES);
}

// Get the throttle position as a percent
// @ret False if implausibility, true otherwise
bool getThrottlePositionPercent(float *throttleOut)
{
    float throttle1_percent, throttle2_percent;
    float throttle;
    (*throttleOut) = 0;

    float thA = getThrottleAFiltered();
    float thB = getThrottleBFiltered();
    float brake = brakeThrottleSteeringADCVals[BRAKE_POS_INDEX];

    ThrottleAReading = thA;
    ThrottleBReading = thB;
    BrakeReading = brake;
    // DEBUG_PRINT("ThA: %u, ThB: %u, Brake: %u\n", (uint16_t)ThrottleAReading >> 3, (uint16_t)ThrottleBReading >> 3, (uint16_t)BrakeReading >> 3);
    sendCAN_VCU_ADCReadings();

    
    // Read both TPS sensors
    if (is_throttle1_in_range(thA)
        && is_throttle2_in_range(thB))
    {
        throttle1_percent = calculate_throttle_percent1(thA);
        throttle2_percent = calculate_throttle_percent2(thB);
    } else {
      ERROR_PRINT("Throttle pot out of range: (A: %lu, B: %lu)\n", (uint32_t)thA, (uint32_t)thB);
      return false;
    }

    // Check if two throttle pots agree
    if(!is_tps_within_tolerance(throttle1_percent, throttle2_percent))
    {
        (*throttleOut) = 0;
        ERROR_PRINT("implausible pedal! difference: %f %%\r\n", throttle1_percent - throttle2_percent);
        DEBUG_PRINT("Throttle A: %lu, Throttle B: %lu\n", (uint32_t)thA, (uint32_t)thB);
        return false;
    } else {
        /*DEBUG_PRINT("t1 %ld, t2 %ld\n", throttle1_percent, throttle2_percent);*/
        throttle = (throttle1_percent + throttle2_percent) / TPS_SENSOR_COUNT;
    }

    *throttleOut = throttle;
    return true;
}

/*
 * Checks brake position range, then cross-checks brake position % against brake pressure %.
 * If the pedal says brakes are engaged but pressure isn't building,
 * the hydraulic system is likely failed — sends EV_Brake_Pressure_Fault.
 * Returns true if implausibility detected.
 */
static bool checkBrakeImplausibility()
{
    float brakePos = getBrakePosFiltered();
    if (!isBrakePositionRawInRange(brakePos)) {
        ERROR_PRINT("Brake position out of range: %.1f [%.1f, %.1f]\n",
                    brakePos, (float)BRAKE_POS_LOW, (float)BRAKE_POS_HIGH);
        fsmSendEventUrgent(&VCUFsmHandle, EV_Brake_Position_Fault, portMAX_DELAY);
        return true;
    }

    float posPercent = getBrakePotentiometerPercentFromRaw(brakePos);
    float presPercent = getBrakePressurePercent();
    if (!brakePositionAndPressureAgree(posPercent, presPercent)) {
        ERROR_PRINT("Brake implausibility: pos=%.1f%% pres=%.1f%%, tolerance=%.1f%%\n",
                    posPercent, presPercent, BRAKE_POSITION_PRESSURE_TOLERANCE_PERCENT);
        fsmSendEventUrgent(&VCUFsmHandle, EV_Brake_Pressure_Fault, portMAX_DELAY);
        return true;
    }
    return false;
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

    if (appsBrakePedalPlausibilityCheckFail(throttle)) {
        (*throttleOut) = 0;
        return THROTTLE_DISABLED;
    }

    // If we get here, all checks have passed, so can safely output throttle
    (*throttleOut) = throttle;

    return THROTTLE_OK;
}

/* Public Functions */
bool isBrakePressed()
{
    /*DEBUG_PRINT("Brake %f\n", getBrakePositionPercent());*/

    if (getBrakePositionPercent() > MIN_BRAKE_PRESSED_VAL_PERCENT) {
        return true;
    } else {
        return false;
    }
}

bool appsBrakePedalPlausibilityCheckFail(float throttle)
{
    // Flag set if throttle and brake pressed at same time
    static bool throttleAndBrakePressedError = false;
    if (throttleAndBrakePressedError) {
        // Both throttle and brake were pressed, check if still the case
        if (throttle < TPS_WHILE_BRAKE_PRESSED_RESET_PERCENT) {
            throttleAndBrakePressedError = false;
            sendDTC_WARNING_BrakeWhileThrottleError_Enabled();
        } else {
            DEBUG_PRINT("Throttle disabled, brake was pressed and throttle still not zero\n");
            return true;
        }
    } else if (getBrakePositionPercent() > APPS_BRAKE_PLAUSIBILITY_THRESHOLD && throttle > TPS_MAX_WHILE_BRAKE_PRESSED_PERCENT) {
        throttleAndBrakePressedError = true;
        sendDTC_WARNING_BrakeWhileThrottleError_Disabled();
        DEBUG_PRINT("Throttle disabled, brakePressed\n");
        return true;
    }

    return false;
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
  // TODO: Monitor brake pressure sensor (BPS) for failures
  return true;
}

static float getBrakePressurePSI()
{
    float raw = getBrakePresFiltered();
    float v = raw / ADC_MAX_VALUE * BRAKE_PRESSURE_ADC_REF_V * BRAKE_PRESSURE_VOLTAGE_DIVIDER_SCALE;
    return map_range_float(v,
                           BRAKE_PRESSURE_SENSOR_MIN_V,
                           BRAKE_PRESSURE_SENSOR_MAX_V,
                           BRAKE_PRESSURE_SENSOR_MIN_PSI,
                           BRAKE_PRESSURE_SENSOR_MAX_PSI);
}

int getBrakePressure() {
    return (int)getBrakePressurePSI();
}

float getBrakePressurePercent() {
    return map_range_float(getBrakePresFiltered(), BRAKE_PRES_LOW, BRAKE_PRES_HIGH, PERCENT_MIN, PERCENT_MAX);
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

HAL_StatusTypeDef pollThrottle(float *throttlePercentReading) {
    ThrottleStatus_t rc = getNewThrottle(throttlePercentReading);

    if (rc == THROTTLE_FAULT) {
        sendDTC_WARNING_Throttle_Failure(THROTTLE_FAILURE_DTC_DETAIL);
        DEBUG_PRINT("Throttle value out of range\n");
        return HAL_ERROR;
    }

    if(!isLockoutDisabled()) {
        // Send lockout release to MC
        if (sendLockoutReleaseToMC() != HAL_OK) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

bool regenEnabled = false;
void toggleRegen(){
    regenEnabled = !regenEnabled;
}

bool isRegenEnabled() {
    return regenEnabled;
}

void disableRegen() {
    regenEnabled = false;
    // DEBUG_PRINT("Regen disabled\n");
    ENDURANCE_LED_OFF;
}

/*********************************************************************************************************************/
/*----------------------------------------------------Tasks----------------------------------------------------------*/
/*********************************************************************************************************************/

void canPublishTask(void *pvParameters)
{
    // Delay to allow first ADC readings to come in
    vTaskDelay(pdMS_TO_TICKS(VCU_DATA_STARTUP_DELAY_MS));
    while (1) {
        // Update value to be sent over can
        ThrottlePercent = throttlePercentReading;
        FrontBrakePressure = getBrakePressure();
        SteeringAngle = getSteeringAngle();
        BrakePercent = getBrakePositionPercent();
        // DEBUG_PRINT("Pres: %d\n",(int) brakePressure);

        if (sendCAN_VCU_Data() != HAL_OK) {
            ERROR_PRINT("Failed to send vcu can data\n");
        }
        vTaskDelay(pdMS_TO_TICKS(VCU_DATA_PUBLISH_TIME_MS));
    }
}


/**
 * @brief  Commands the inverter with motoring or regenerative torque
 *         Polls the throttle and brakes to determine the torque command
 */
void InvCommandTask(void) 
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    if (registerTaskToWatch(INV_COMMAND_TASK_ID, INV_COMMAND_WATCHDOG_PERIOD_MULTIPLIER * pdMS_TO_TICKS(INV_COMMAND_TASK_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("ERROR: Failed to init inverter command task, suspending throttle polling task\n");
        Error_Handler();
    }

    INV_Peak_Tractive_Power_kW = 0;
    INV_DC_Bus_Voltage = 0;
    INV_DC_Bus_Current = 0;
    InvCommandMode_t commandMode = MOTORING;
    while (1)
    {
        // Once EM Enabled, start polling throttle
        if (fsmGetState(&VCUFsmHandle) == STATE_EM_Enable)
        {
            float requestTorque = 0;
            // Poll throttle
            if (pollThrottle(&throttlePercentReading) != HAL_OK) {
                ERROR_PRINT("ERROR: Failed to poll throttle\n");
                fsmSendEventUrgent(&VCUFsmHandle, EV_BTN_HV_Toggle, portMAX_DELAY);
            }

            // Check brake pos vs pressure: large discrepancy means hydraulic failure
            checkBrakeImplausibility();

            // poll brake
            float brakePercent = getBrakePositionPercent();
            float kph = INV_Motor_Speed * RPM_TO_KPH;

            // EV.3.3.3 < 5kmh = no regen
            if (kph > MIN_KPH_FOR_REGEN && isRegenEnabled() && brakePercent > MIN_BRAKE_PERCENT_FOR_REGEN_TORQUE) {
                requestTorque = mapBrakeToRegenTorque(brakePercent);
                commandMode = REGEN;
            } else {
                requestTorque = mapThrottleToTorque(throttlePercentReading);
                commandMode = MOTORING;
            }
            // DEBUG_PRINT("A: %.2f, B: %.2f, T: %.1f, M: %d\n", throttlePercentReading, brakePercent, requestTorque, commandMode);
            if (requestTorqueFromMC(requestTorque, commandMode) != HAL_OK) {
                ERROR_PRINT("ERROR: Failed to request torque from MC\n");
                fsmSendEventUrgent(&VCUFsmHandle, EV_BTN_HV_Toggle, portMAX_DELAY);
            }
        }
        else
        {
            // EM disabled
            // still update throttle ADC moving filter
            float thA = getThrottleAFiltered();
            float thB = getThrottleBFiltered();
            float brake = brakeThrottleSteeringADCVals[BRAKE_POS_INDEX];

            ThrottleAReading = thA;
            ThrottleBReading = thB;
            BrakeReading = brake;
            // DEBUG_PRINT("ThA: %u, ThB: %u, Brake: %u\n", (uint16_t)ThrottleAReading >> 3, (uint16_t)ThrottleBReading >> 3, (uint16_t)BrakeReading >> 3);
            sendCAN_VCU_ADCReadings();
            
            throttlePercentReading = 0;
        }
        watchdogTaskCheckIn(INV_COMMAND_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(INV_COMMAND_TASK_PERIOD_MS));
    }
}
