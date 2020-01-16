#ifndef MOTORCONTROLLER_H

#define MOTORCONTROLLER_H

#include "bsp.h"

#define MAX_TORQUE_DEMAND_DEFAULT 3
#define BRAKING_TORQUE_LIMIT_DEFAULT 0
#define SPEED_LIMIT_DEFAULT 4000
#define REVERSE_SPEED_LIMIT_DEFAULT 0
#define DISCHARGE_CURRENT_LIMIT_DEFAULT 100
#define CHARGE_CURRENT_LIMIT_DEFAULT 0
#define HIGH_VOLTAGE_LIMIT_DEFAULT 400
#define LOW_VOLTAGE_LIMIT_DEFAULT 200

#define INVERTER_STARTUP_TIMEOUT_MS 10000 // TODO: Chose a good value for this
#define INVERTER_STOP_TIMEOUT_MS    10000 // TODO: Chose a good value for this

typedef struct MotorControllerProcanSettings {
    uint64_t InverterCommand;
    float DriveTorqueLimit;
    float BrakingTorqueLimit;
    float ForwardSpeedLimit;
    float ReverseSpeedLimit;
    float DischargeCurrentLimit;
    float ChargeCurrentLimit;
    float HighVoltageLimit;
    float LowVoltageLimit;
    float MaxTorqueDemand;
} MotorControllerProcanSettings;

extern uint64_t maxTorqueDemand;

HAL_StatusTypeDef mcInit();
HAL_StatusTypeDef sendThrottleValueToMCs(float throttle);
HAL_StatusTypeDef mcShutdown();
HAL_StatusTypeDef initMotorControllerProcanSettings();
HAL_StatusTypeDef setMotorControllerProcanSettings(MotorControllerProcanSettings settings);
HAL_StatusTypeDef setForwardSpeedLimit(float limit);
HAL_StatusTypeDef setTorqueLimit(float limit);

#endif /* end of include guard: MOTORCONTROLLER_H */
