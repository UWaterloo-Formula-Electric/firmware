#ifndef MOTORCONTROLLER_H

#define MOTORCONTROLLER_H

#include "bsp.h"
#include "canReceive.h"

#define MIN_THROTTLE_PERCENT_FOR_TORQUE 5.0f // If under 5% throttle pedal don't request torque

#define MAX_TORQUE_DEMAND_DEFAULT_NM    200 
#define MAX_MOTOR_TORQUE_NM             231
#define SPEED_LIMIT_DEFAULT             10000
#define DISCHARGE_CURRENT_LIMIT_DEFAULT 250
#define CHARGE_CURRENT_LIMIT_DEFAULT    0

#define INVERTER_STOP_TIMEOUT_MS        10000   // TODO: Chose a good value for this
#define MC_INIT_DISCHARGE_TIME_MS       1000

#define INVERTER_LOCKOUT_ENABLED        0x1
#define INVERTER_LOCKOUT_DISABLED       0x0
#define INVERTER_DIRECTION_FORWARD      0x0
#define INVERTER_DIRECTION_REVERSE      0x1
#define INVERTER_ON                     0x1
#define INVERTER_OFF                    0x0
#define INVERTER_DISCHARGE_ENABLE       0x1
#define INVERTER_DISCHARGE_DISABLE      0x0
#define SPEED_MODE_OVERRIDE_FALSE       0x0
#define TORQUE_LIMIT_OVERRIDE_FALSE     0x0
#define TORQUE_MODE_SPEED_REQUEST       0x0
#define INVERTER_PARAM_WRITE            0x1
#define INVERTER_PARAM_READ             0x0

#define INVERTER_FAULT_CLEAR_ADDRESS    20

#define W_TO_KW (1.0f/1000.0f)

typedef struct MotorControllerSettings {
    bool InverterMode;
    float DriveTorqueLimit; // Adjustable torque limit
    float ForwardSpeedLimit;
    float DischargeCurrentLimit;
    float ChargeCurrentLimit;
    float MaxTorqueDemand; // Motor max torque (datasheet)
    bool DirectionCommand;
} MotorControllerSettings;

extern uint64_t maxTorqueDemand;

HAL_StatusTypeDef mcInit();
HAL_StatusTypeDef requestTorqueFromMC(float throttle_percent);
HAL_StatusTypeDef sendLockoutReleaseToMC();
HAL_StatusTypeDef mcClearFaults();
HAL_StatusTypeDef sendDisableMC();
HAL_StatusTypeDef initMotorControllerSettings();
HAL_StatusTypeDef setMotorControllerSettings(MotorControllerSettings settings);
HAL_StatusTypeDef setDischargeCurrentLimit(float limit);
HAL_StatusTypeDef setForwardSpeedLimit(float limit);
HAL_StatusTypeDef setTorqueLimit(float limit);
HAL_StatusTypeDef mcWriteParamCommand(uint16_t address, uint16_t data);
HAL_StatusTypeDef mcReadParamCommand(uint16_t address, uint16_t data);

#endif /* end of include guard: MOTORCONTROLLER_H */
