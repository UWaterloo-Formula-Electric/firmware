#ifndef MOTORCONTROLLER_H

#define MOTORCONTROLLER_H

#include "bsp.h"

#define MAX_TORQUE_DEMAND_DEFAULT       30
#define BRAKING_TORQUE_LIMIT_DEFAULT    0
#define SPEED_LIMIT_DEFAULT             10000
#define REVERSE_SPEED_LIMIT_DEFAULT     0
#define DISCHARGE_CURRENT_LIMIT_DEFAULT 250
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

HAL_StatusTypeDef mcInit(void);
HAL_StatusTypeDef sendThrottleValueToMCs(float throttle, int steeringAngle);
HAL_StatusTypeDef mcShutdown(void);
HAL_StatusTypeDef initMotorControllerProcanSettings(void);
HAL_StatusTypeDef setMotorControllerProcanSettings(MotorControllerProcanSettings settings);
HAL_StatusTypeDef setDischargeCurrentLimit(float limit);
HAL_StatusTypeDef setForwardSpeedLimit(float limit);
HAL_StatusTypeDef setTorqueLimit(float limit);
void set_tv_deadzone_end_right(float tv_deadzone_end_right_value);
void set_tv_deadzone_end_left(float tv_deadzone_end_left_value);
void set_torque_vector_factor(float torque_vector_factor_value);
void set_max_torque_demand(float max_torque_demand_default_value);
float get_torque_vector_factor(void);
float get_max_torque_demand(void);
float get_tv_deadzone_end_right(void);
float get_tv_deadzone_end_left(void);

#endif /* end of include guard: MOTORCONTROLLER_H */
