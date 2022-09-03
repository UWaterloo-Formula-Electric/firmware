#ifndef MOTORCONTROLLER_H

#define MOTORCONTROLLER_H

#include "bsp.h"

#define MAX_TORQUE_DEMAND_DEFAULT       20
#define BRAKING_TORQUE_LIMIT_DEFAULT    0
#define SPEED_LIMIT_DEFAULT             10000
#define REVERSE_SPEED_LIMIT_DEFAULT     0
#define DISCHARGE_CURRENT_LIMIT_DEFAULT 30
#define CHARGE_CURRENT_LIMIT_DEFAULT    0
#define HIGH_VOLTAGE_LIMIT_DEFAULT      300
#define LOW_VOLTAGE_LIMIT_DEFAULT       100

#define INVERTER_STOP_TIMEOUT_MS        10000   // TODO: Chose a good value for this
#define MC_INIT_DISCHARGE_TIME_MS       1000

#define TORQUE_VECTOR_FACTOR            (0.25f/30.0f)

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
HAL_StatusTypeDef sendThrottleValueToMCs(float throttle, int steeringAngle);
HAL_StatusTypeDef mcShutdown();
HAL_StatusTypeDef initMotorControllerProcanSettings();
HAL_StatusTypeDef setMotorControllerProcanSettings(MotorControllerProcanSettings settings);
HAL_StatusTypeDef setForwardSpeedLimit(float limit);
HAL_StatusTypeDef setTorqueLimit(float limit);

#endif /* end of include guard: MOTORCONTROLLER_H */
