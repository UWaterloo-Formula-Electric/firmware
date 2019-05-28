#include "motorController.h"
#include "bsp.h"
#include "VCU_f7_can.h"
#include "debug.h"
#include "task.h"

// MC Questions:
// Do we need to wait to close contactors until MCs are ready?
// How to read IDs for msgs on datasheet?

#define INVERTER_STARTUP_TIMEOUT 1000 // TODO: Chose a good value for this

uint64_t maxTorqueDemand = 0;

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
    InverterCommandRight = commandVal;
    // TODO: What to set these too when sending the command
    SpeedLimitForwardRight = 0;
    SpeedLimitReverseRight = 0;
    return sendCAN_SpeedLimitRight();
}

HAL_StatusTypeDef mcLeftCommand(uint16_t commandVal)
{
    InverterCommandLeft = commandVal;
    // TODO: What to set these too when sending the command
    SpeedLimitForwardLeft = 0;
    SpeedLimitReverseLeft = 0;
    return sendCAN_SpeedLimitLeft();
}

HAL_StatusTypeDef initSpeedAndTorqueLimits()
{
    SpeedLimitForwardLeft = SPEED_LIMIT_DEFAULT;
    SpeedLimitReverseLeft = SPEED_LIMIT_DEFAULT;
    SpeedLimitForwardRight = SPEED_LIMIT_DEFAULT;
    SpeedLimitReverseRight = SPEED_LIMIT_DEFAULT;
    maxTorqueDemand = MAX_TORQUE_DEMAND_DEFAULT;

    return HAL_OK;
}

// TODO: Probably need to set speed limits after init
HAL_StatusTypeDef mcInit()
{
    if (mcRightCommand(0x4) != HAL_OK) {
        ERROR_PRINT("Failed to send init disable bridge command to MC Right");
        return HAL_ERROR;
    }

    if (mcLeftCommand(0x4) != HAL_OK) {
        ERROR_PRINT("Failed to send init disable bridge command to MC Left");
        return HAL_ERROR;
    }

    // TODO: Do we need to check the voltage values from the MC, or are the BMU
    // startup checks sufficient?


    uint32_t startTick = xTaskGetTickCount();
    while (xTaskGetTickCount() - startTick < INVERTER_STARTUP_TIMEOUT &&
           StateInverterRight != 0x18);

    if (StateInverterRight != 0x18) {
        ERROR_PRINT("Timeout waiting for MC Right to be ready to turn on\n");
        return HAL_TIMEOUT;
    }

    if (mcRightCommand(0x1) != HAL_OK) {
        ERROR_PRINT("Failed to send enable bridge command to MC Right");
        return HAL_ERROR;
    }

    startTick = xTaskGetTickCount();
    while (xTaskGetTickCount() - startTick < INVERTER_STARTUP_TIMEOUT &&
           StateInverterLeft != 0x18);

    if (StateInverterLeft != 0x18) {
        ERROR_PRINT("Timeout waiting for MC Right to be ready to turn on\n");
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

    if (mcRightCommand(0x4) != HAL_OK) {
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

HAL_StatusTypeDef sendThrottleValueToMCs(float throttle)
{
    float minTorqueAvailable = min(TorqueAvailableDriveRight, TorqueAvailableDriveLeft);

    float torqueDemand = map_range_float(throttle, 0, 100, 0, maxTorqueDemand);

    torqueDemand = limit(torqueDemand, 0, minTorqueAvailable);

    TorqueLimitDriveRight = TorqueAvailableDriveRight;
    TorqueLimitBrakingRight = 0; // TODO: What to do for braking torque
    TorqueDemandRight = torqueDemand;

    TorqueLimitDriveLeft = TorqueAvailableDriveLeft;
    TorqueLimitBrakingRight = 0; // TODO: What to do for braking torque
    TorqueDemandLeft = torqueDemand;

    if (sendCAN_TorqueLimitRight() != HAL_OK) {
        ERROR_PRINT("Failed to send torque limit right\n");
        return HAL_ERROR;
    }
    if (sendCAN_TorqueLimitLeft() != HAL_OK) {
        ERROR_PRINT("Failed to send torque limit right\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}
