#include "motorController.h"
#include "bsp.h"
#include "VCU_F7_can.h"
#include "debug.h"
#include "task.h"
#include "string.h"
#include "inttypes.h"
#include "drive_by_wire.h"

// MC Questions:
// Do we need to wait to close contactors until MCs are ready?
// How to read IDs for msgs on datasheet?

#define INVERTER_STATE_MASK 0x3F

MotorControllerProcanSettings mcLeftSettings = {0};
MotorControllerProcanSettings mcRightSettings = {0};

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

// TODO: Probably need to set speed limits after init
HAL_StatusTypeDef mcInit()
{
    /*if (mcRightCommand(0x4) != HAL_OK) {*/
        /*ERROR_PRINT("Failed to send init disable bridge command to MC Right");*/
        /*return HAL_ERROR;*/
    /*}*/

    if (mcLeftCommand(0x4) != HAL_OK) {
        ERROR_PRINT("Failed to send init disable bridge command to MC Left");
        return HAL_ERROR;
    }

    DEBUG_PRINT("Waiting for MC to complete startup checks\n");

    // TODO: Do we need to check the voltage values from the MC, or are the BMU
    // startup checks sufficient?


    uint32_t startTick = xTaskGetTickCount();
    /*while (xTaskGetTickCount() - startTick < INVERTER_STARTUP_TIMEOUT_MS &&*/
           /*StateInverterRight != 0x18);*/

    /*if (StateInverterRight != 0x18) {*/
        /*ERROR_PRINT("Timeout waiting for MC Right to be ready to turn on\n");*/
        /*return HAL_TIMEOUT;*/
    /*}*/

    /*if (mcRightCommand(0x1) != HAL_OK) {*/
        /*ERROR_PRINT("Failed to send enable bridge command to MC Right");*/
        /*return HAL_ERROR;*/
    /*}*/

    startTick = xTaskGetTickCount();
    while (xTaskGetTickCount() - startTick < INVERTER_STARTUP_TIMEOUT_MS &&
           (StateInverterLeft & INVERTER_STATE_MASK) != 0x18)
    {
        sendThrottleValueToMCs(0);
        vTaskDelay(pdMS_TO_TICKS(THROTTLE_POLL_TIME_MS));
    }

    if ((StateInverterLeft & INVERTER_STATE_MASK) != 0x18) {
        ERROR_PRINT("Timeout waiting for MC Left to be ready to turn on\n");
        return HAL_TIMEOUT;
    }

    if (mcLeftCommand(0x1) != HAL_OK) {
        ERROR_PRINT("Failed to send enable bridge command to MC Left");
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef mcShutdown()
{
    if (mcLeftCommand(0x4) != HAL_OK) {
        ERROR_PRINT("Failed to send init disable bridge command to MC Left");
        return HAL_ERROR;
    }

    /*if (mcRightCommand(0x4) != HAL_OK) {*/
        /*ERROR_PRINT("Failed to send init disable bridge command to MC Right");*/
        /*return HAL_ERROR;*/
    /*}*/

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

HAL_StatusTypeDef sendThrottleValueToMCs(float throttle)
{
    float maxTorqueDemand = min(mcRightSettings.DriveTorqueLimit, mcLeftSettings.DriveTorqueLimit);

    float torqueDemand = map_range_float(throttle, 0, 100, 0, maxTorqueDemand);

    static uint64_t count2 = 0;
    count2++;
    if (count2 % 20 == 0) {
        DEBUG_PRINT("Torque demand %f\n", torqueDemand);
    }

    TorqueLimitDriveRight = mcRightSettings.DriveTorqueLimit;
    TorqueLimitBrakingRight = mcRightSettings.BrakingTorqueLimit;
    TorqueDemandRight = torqueDemand;

    TorqueLimitDriveLeft = mcLeftSettings.DriveTorqueLimit;
    TorqueLimitBrakingLeft = mcLeftSettings.BrakingTorqueLimit;
    TorqueDemandLeft = torqueDemand;

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
    ActiveShortRight = 0;

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
