#include "motorController.h"
#include "bsp.h"
#include "vcu_F7_can.h"
#include "debug.h"
#include "task.h"
#include "string.h"
#include "inttypes.h"
#include "drive_by_wire.h"

// MC Questions:
// Do we need to wait to close contactors until MCs are ready?
// How to read IDs for msgs on datasheet?

/* Control Values - See 3.4.1 in ProCAN fundementals */
#define INVERTER_NOP                    0x00
#define INVERTER_ENABLE_BRIDGE          0x01
#define INVERTER_RESET                  0x07
#define INVERTER_DISABLE_BRIDGE         0x04
#define INVERTER_DISCHARGE_DC_LINK      0x02

/* Status Values - See 3.4.2 in ProCAN fundementals */
#define INVERTER_BRIDGE_DISABLED        0x18
#define INVERTER_BRIDGE_ENABLED         0x1A
#define INVERTER_SELF_TEST              0x15
#define INVERTER_FAULT_OFF              0x25
#define INVERTER_NOT_READY              0x00

#define INVERTER_STATE_MASK             0x3F

// Torque vectoring dead zone angle boundaries
#define TV_DEADZONE_END_RIGHT 10
#define TV_DEADZONE_END_LEFT -10

MotorControllerProcanSettings mcLeftSettings = {0};
MotorControllerProcanSettings mcRightSettings = {0};
// This is bad but a band-aid, we should really create a good way and check inverter status
volatile bool motors_active = false;

float min(float a, float b)
{
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

HAL_StatusTypeDef mcRightCommand(uint16_t commandVal)
{
	
    mcRightSettings.InverterCommand = commandVal;
    InverterCommandRight = mcRightSettings.InverterCommand;
    SpeedLimitForwardRight = mcRightSettings.ForwardSpeedLimit;
    SpeedLimitReverseRight = mcRightSettings.ReverseSpeedLimit;
    return sendCAN_SpeedLimitRight();
}

HAL_StatusTypeDef mcLeftCommand(uint16_t commandVal)
{
    mcLeftSettings.InverterCommand = commandVal;
    InverterCommandLeft = mcLeftSettings.InverterCommand;
    SpeedLimitForwardLeft = mcLeftSettings.ForwardSpeedLimit;
    SpeedLimitReverseLeft = mcLeftSettings.ReverseSpeedLimit;
    return sendCAN_SpeedLimitLeft();
}

HAL_StatusTypeDef initMotorControllerProcanSettings()
{
    mcLeftSettings.InverterCommand = 0;
    mcLeftSettings.DriveTorqueLimit = MAX_TORQUE_DEMAND_DEFAULT;
    mcLeftSettings.BrakingTorqueLimit = BRAKING_TORQUE_LIMIT_DEFAULT;
    mcLeftSettings.ForwardSpeedLimit = SPEED_LIMIT_DEFAULT;
    mcLeftSettings.ReverseSpeedLimit = SPEED_LIMIT_DEFAULT;
    mcLeftSettings.DischargeCurrentLimit = DISCHARGE_CURRENT_LIMIT_DEFAULT;
    mcLeftSettings.ChargeCurrentLimit = CHARGE_CURRENT_LIMIT_DEFAULT;
    mcLeftSettings.HighVoltageLimit = HIGH_VOLTAGE_LIMIT_DEFAULT;
    mcLeftSettings.LowVoltageLimit = LOW_VOLTAGE_LIMIT_DEFAULT;
    mcLeftSettings.MaxTorqueDemand = MAX_TORQUE_DEMAND_DEFAULT;

    mcRightSettings.InverterCommand = 0;
    mcRightSettings.DriveTorqueLimit = MAX_TORQUE_DEMAND_DEFAULT;
    mcRightSettings.BrakingTorqueLimit = BRAKING_TORQUE_LIMIT_DEFAULT;
    mcRightSettings.ForwardSpeedLimit = SPEED_LIMIT_DEFAULT;
    mcRightSettings.ReverseSpeedLimit = SPEED_LIMIT_DEFAULT;
    mcRightSettings.DischargeCurrentLimit = DISCHARGE_CURRENT_LIMIT_DEFAULT;
    mcRightSettings.ChargeCurrentLimit = CHARGE_CURRENT_LIMIT_DEFAULT;
    mcRightSettings.HighVoltageLimit = HIGH_VOLTAGE_LIMIT_DEFAULT;
    mcRightSettings.LowVoltageLimit = LOW_VOLTAGE_LIMIT_DEFAULT;
    mcRightSettings.MaxTorqueDemand = MAX_TORQUE_DEMAND_DEFAULT;

    return HAL_OK;
}

HAL_StatusTypeDef setMotorControllerProcanSettings(MotorControllerProcanSettings settings)
{
    memcpy(&mcLeftSettings, &settings, sizeof(settings));
    memcpy(&mcRightSettings, &settings, sizeof(settings));

    return HAL_OK;
}

HAL_StatusTypeDef setForwardSpeedLimit(float limit)
{
    mcLeftSettings.ForwardSpeedLimit = limit;
    mcRightSettings.ForwardSpeedLimit = limit;

    return HAL_OK;
}

HAL_StatusTypeDef setTorqueLimit(float limit)
{
    mcRightSettings.DriveTorqueLimit = limit;
    mcLeftSettings.DriveTorqueLimit = limit;

    return HAL_OK;
}

HAL_StatusTypeDef setDischargeCurrentLimit(float limit)
{
    mcRightSettings.DischargeCurrentLimit = limit;
    mcLeftSettings.DischargeCurrentLimit = limit;
	return HAL_OK;
}

// TODO: Probably need to set speed limits after init
HAL_StatusTypeDef mcInit()
{
	DEBUG_PRINT("Discharging MCs for 1 second before turning on\n");

    if (mcRightCommand(INVERTER_DISCHARGE_DC_LINK) != HAL_OK) {
        ERROR_PRINT("Failed to send discharge command to MC Right");
        return HAL_ERROR;
    }

    if (mcLeftCommand(INVERTER_DISCHARGE_DC_LINK) != HAL_OK) {
        ERROR_PRINT("Failed to send discharge command to MC Left");
        return HAL_ERROR;
    }

    vTaskDelay(pdMS_TO_TICKS(MC_INIT_DISCHARGE_TIME_MS));

    if (mcRightCommand(INVERTER_DISABLE_BRIDGE) != HAL_OK) {
        ERROR_PRINT("Failed to send init disable bridge command to MC Right");
        return HAL_ERROR;
    }

    if (mcLeftCommand(INVERTER_DISABLE_BRIDGE) != HAL_OK) {
        ERROR_PRINT("Failed to send init disable bridge command to MC Left");
        return HAL_ERROR;
    }

    DEBUG_PRINT("Waiting for MC to complete startup checks\n");

    // TODO: Do we need to check the voltage values from the MC, or are the BMU
    // startup checks sufficient?

    DEBUG_PRINT("Starting MC Left...\n");

    uint32_t startTick = xTaskGetTickCount();
    while ((xTaskGetTickCount() - startTick < (INVERTER_ON_TIMEOUT_MS)) &&
           ((StateInverterLeft & INVERTER_STATE_MASK) != INVERTER_BRIDGE_DISABLED))
    {
        sendThrottleValueToMCs(0, 0);
        vTaskDelay(pdMS_TO_TICKS(THROTTLE_POLL_TIME_MS));
    }

    if ((StateInverterLeft & INVERTER_STATE_MASK) != INVERTER_BRIDGE_DISABLED) {
        ERROR_PRINT("Timeout waiting for MC Left to be ready to turn on\n");
        ERROR_PRINT("StateInverterLeft %d\n", (uint8_t)(StateInverterLeft & INVERTER_STATE_MASK));
        return HAL_TIMEOUT;
    }

    DEBUG_PRINT("Left motor controller ready.\n");

    DEBUG_PRINT("Starting MC Right...\n");

    startTick = xTaskGetTickCount();
    while ((xTaskGetTickCount() - startTick < (INVERTER_ON_TIMEOUT_MS)) &&
           ((StateInverterRight & INVERTER_STATE_MASK) != INVERTER_BRIDGE_DISABLED))
    {
        sendThrottleValueToMCs(0, 0);
        vTaskDelay(pdMS_TO_TICKS(THROTTLE_POLL_TIME_MS));
    }

    if ((StateInverterRight & INVERTER_STATE_MASK) != INVERTER_BRIDGE_DISABLED) {
        ERROR_PRINT("Timeout waiting for MC Right to be ready to turn on\n");
        ERROR_PRINT("StateInverterRight %d\n", (uint8_t)(StateInverterRight & INVERTER_STATE_MASK));
        return HAL_TIMEOUT;
    }

    DEBUG_PRINT("Right motor controller ready.\n");

    DEBUG_PRINT("Initializing default settings...");
    initMotorControllerProcanSettings();

    if (mcLeftCommand(INVERTER_ENABLE_BRIDGE) != HAL_OK) {
        ERROR_PRINT("Failed to send enable bridge command to MC Left");
        return HAL_ERROR;
    }

    if (mcRightCommand(INVERTER_ENABLE_BRIDGE) != HAL_OK) {
        ERROR_PRINT("Failed to send enable bridge command to MC right");
        return HAL_ERROR;
    }
    motors_active = true;

    return HAL_OK;
}

HAL_StatusTypeDef mcShutdown()
{
	motors_active = false;
	// Wait for final throttle messages to exit our CAN queue
    vTaskDelay(pdMS_TO_TICKS(250));
    if (mcLeftCommand(INVERTER_DISABLE_BRIDGE) != HAL_OK) {
        ERROR_PRINT("Failed to send init disable bridge command to MC Left");
        return HAL_ERROR;
    }

    if (mcRightCommand(INVERTER_DISABLE_BRIDGE) != HAL_OK) {
        ERROR_PRINT("Failed to send init disable bridge command to MC Right");
        return HAL_ERROR;
    }

    return HAL_OK;
}

float map_range_float(float in, float low, float high, float low_out, float high_out) {
    if (in < low) {
        in = low;
    } else if (in > high) {
        in = high;
    }
    float in_range = high - low;
    float out_range = high_out - low_out;

    return (in - low) * out_range / in_range + low_out;
}

float limit(float in, float min, float max)
{
    if (in > max) {
        in = max;
    } else if (in < min) {
        in = min;
    }

    return min;
}

HAL_StatusTypeDef sendThrottleValueToMCs(float throttle, int steeringAngle)
{

	if(!motors_active)
	{
        
		return HAL_OK;
	}

    float maxTorqueDemand = min(mcRightSettings.DriveTorqueLimit, mcLeftSettings.DriveTorqueLimit);

	float throttleRight = throttle - (throttle * steeringAngle * TORQUE_VECTOR_FACTOR);
	float throttleLeft = throttle + (throttle * steeringAngle * TORQUE_VECTOR_FACTOR);

    float torqueDemandR = map_range_float(throttleRight, 0, 100, 0, maxTorqueDemand);
    float torqueDemandL = map_range_float(throttleLeft, 0, 100, 0, maxTorqueDemand);

    TorqueLimitDriveRight = mcRightSettings.DriveTorqueLimit;
    TorqueLimitBrakingRight = mcRightSettings.BrakingTorqueLimit;
    TorqueDemandRight = torqueDemandR;

    TorqueLimitDriveLeft = mcLeftSettings.DriveTorqueLimit;
    TorqueLimitBrakingLeft = mcLeftSettings.BrakingTorqueLimit;
    TorqueDemandLeft = torqueDemandL;

    SpeedLimitForwardRight = mcRightSettings.ForwardSpeedLimit;
    SpeedLimitReverseRight = mcRightSettings.ReverseSpeedLimit;
    InverterCommandRight = mcRightSettings.InverterCommand; // Keep sending enable bridge command

    SpeedLimitForwardLeft = mcLeftSettings.ForwardSpeedLimit;
    SpeedLimitReverseLeft = mcLeftSettings.ReverseSpeedLimit;
    InverterCommandLeft = mcLeftSettings.InverterCommand; // Keep sending enable bridge command

    CurrentLimitDschrgInverterRight = mcRightSettings.DischargeCurrentLimit;
    CurrentLimitChargeInverterRight = mcRightSettings.ChargeCurrentLimit;;

    CurrentLimitDschrgInverterLeft = mcLeftSettings.DischargeCurrentLimit;
    CurrentLimitChargeInverterLeft = mcLeftSettings.ChargeCurrentLimit;;

    VoltageLimitHighInverterRight = mcRightSettings.HighVoltageLimit;
    VoltageLimitLowInverterRight = mcRightSettings.LowVoltageLimit;;

    VoltageLimitHighInverterLeft = mcLeftSettings.HighVoltageLimit;
    VoltageLimitLowInverterLeft = mcLeftSettings.LowVoltageLimit;;

    // Sevcon didn't explain what these are for, but said to set to 0
    ActiveShortRight = 0;
    ActiveShortLeft = 0;

    if (sendCAN_TorqueLimitRight() != HAL_OK) {
        ERROR_PRINT("Failed to send torque limit right\n");
        return HAL_ERROR;
    }
    if (sendCAN_TorqueLimitLeft() != HAL_OK) {
        ERROR_PRINT("Failed to send torque limit left\n");
        return HAL_ERROR;
    }

    if (sendCAN_SpeedLimitRight() != HAL_OK) {
        ERROR_PRINT("Failed to send speed limit right\n");
        return HAL_ERROR;
    }
    if (sendCAN_SpeedLimitLeft() != HAL_OK) {
        ERROR_PRINT("Failed to send speed limit left\n");
        return HAL_ERROR;
    }

    if (sendCAN_CurrentLimitRight() != HAL_OK) {
        ERROR_PRINT("Failed to send current limit right\n");
        return HAL_ERROR;
    }
    if (sendCAN_CurrentLimitLeft() != HAL_OK) {
        ERROR_PRINT("Failed to send current limit left\n");
        return HAL_ERROR;
    }

    if (sendCAN_VoltageLimitRight() != HAL_OK) {
        ERROR_PRINT("Failed to send voltage limit right\n");
        return HAL_ERROR;
    }
    if (sendCAN_VoltageLimitLeft() != HAL_OK) {
        ERROR_PRINT("Failed to send voltage limit left\n");
        return HAL_ERROR;
    }

    if (sendCAN_AuxRight() != HAL_OK) {
        ERROR_PRINT("Failed to send aux right\n");
        return HAL_ERROR;
    }
    if (sendCAN_AuxLeft() != HAL_OK) {
        ERROR_PRINT("Failed to send aux left\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}
