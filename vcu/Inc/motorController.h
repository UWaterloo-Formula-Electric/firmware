#ifndef MOTORCONTROLLER_H

#define MOTORCONTROLLER_H

#include "bsp.h"
#include "canReceive.h"
#include "mathUtils.h"

#define PSI_2_PA 6894.7572f // psi to Pa

#define MIN_KPH_FOR_REGEN 5.f // EV.3.3.3 required by michigan rules, if under this speed, regen cannot be enabled

#define DESIRED_REAR_BRAKE_BIAS_PCT 0.2f // 20% rear brake bias
#define MIN_THROTTLE_PERCENT_FOR_TORQUE 5.0f // If under 5% throttle pedal don't request torque
#define MAX_BRAKE_PERCENT_FOR_REGEN_TORQUE 70.0f // If over this percent, regen torque is maxed out
#define MIN_BRAKE_PERCENT_FOR_REGEN_TORQUE 15.0f // If under 5% brake pedal don't request regen torque

#define MAX_TORQUE_DEMAND_DEFAULT_NM        200
#define MAX_MOTOR_TORQUE_NM                 231
#define MAX_REGEN_TORQUE_DEMAND_DEFAULT_NM  15
#define MAX_REGEN_TORQUE_NM                 15
#define SPEED_LIMIT_DEFAULT                 10000
#define DISCHARGE_CURRENT_LIMIT_DEFAULT     250
#define CHARGE_CURRENT_LIMIT_DEFAULT        0
#define INVERTER_STOP_TIMEOUT_MS            10000   // TODO: Chose a good value for this
#define MC_INIT_DISCHARGE_TIME_MS           1000

#define INVERTER_LOCKOUT_ENABLED        0x1
#define INVERTER_LOCKOUT_DISABLED       0x0
#define INVERTER_DIRECTION_FORWARD      0x1
#define INVERTER_DIRECTION_REVERSE      0x0
#define INVERTER_ON                     0x1
#define INVERTER_OFF                    0x0
#define INVERTER_DISCHARGE_ENABLE       0x1
#define INVERTER_DISCHARGE_DISABLE      0x0
#define SPEED_MODE_OVERRIDE_FALSE       0x0
#define TORQUE_LIMIT_OVERRIDE_FALSE     0x0
#define TORQUE_MODE_SPEED_REQUEST       0x0
#define INVERTER_PARAM_WRITE            0x1
#define INVERTER_PARAM_READ             0x0
#define USE_INV_LIMITS                  (true)
#define INV_TORQUE_REGEN_LIMIT_ENABLED_VALUE  0x0


#define INVERTER_FAULT_CLEAR_ADDRESS    20

#define W_TO_KW (1.0f/1000.0f)
//comment out to remove 80kw power limit

#define ENABLE_POWER_LIMIT
#define INV_POWER_LIMIT 70000.0 //80kw


typedef enum InvCommandMode_t {
    MOTORING = 0,
    REGEN,
} InvCommandMode_t;

typedef struct MotorControllerSettings {
    bool InverterMode;
    float DriveTorqueLimit; // Adjustable torque limit
    float BrakeRegenTorqueDemand; // Adjustable regen torque limit
    float ForwardSpeedLimit;
    float DischargeCurrentLimit;
    float ChargeCurrentLimit;
    float MaxTorqueDemand; // Motor max torque (datasheet)
    float MaxRegenTorqueDemand; // Motor max regen torque (CM200DZ default eeprom value)
    bool DirectionCommand;
} MotorControllerSettings;

HAL_StatusTypeDef mcInit();
HAL_StatusTypeDef requestTorqueFromMC(float requestTorque, InvCommandMode_t commandMode);

HAL_StatusTypeDef sendLockoutReleaseToMC();
HAL_StatusTypeDef mcClearFaults();
HAL_StatusTypeDef sendDisableMC();

HAL_StatusTypeDef initMotorControllerSettings();
HAL_StatusTypeDef setMotorControllerSettings(MotorControllerSettings settings);

HAL_StatusTypeDef setDischargeCurrentLimit(float limit);
HAL_StatusTypeDef setForwardSpeedLimit(float limit);
HAL_StatusTypeDef setTorqueLimit(float limit);
HAL_StatusTypeDef setRegenTorqueLimit(float limit);

float mapThrottleToTorque(float throttle_percent);
float mapBrakeToRegenTorque(float brake_percent);


#endif /* end of include guard: MOTORCONTROLLER_H */
