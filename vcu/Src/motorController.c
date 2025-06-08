/**
 *******************************************************************************
 * @file    motorController.c
 * @author	Unknown
 * @date    Dec 2024
 * @brief   Motor controller initialization, torque command, and write + read
 *          commands
 *
 ******************************************************************************
 */

#include "motorController.h"
#include "brakeAndThrottle.h"
#include "bsp.h"
#include "vcu_F7_can.h"
#include "debug.h"
#include "task.h"
#include "string.h"
#include "inttypes.h"
#include "drive_by_wire.h"
#include "vcu_F7_dtc.h"
#include "mathUtils.h"
#include "wheelConstants.h"

// MC Questions:
// Do we need to wait to close contactors until MCs are ready?
// How to read IDs for msgs on datasheet?



MotorControllerSettings mcSettings = {0};

HAL_StatusTypeDef initMotorControllerSettings()
{
    mcSettings.InverterMode = 0;
    mcSettings.DriveTorqueLimit = MAX_TORQUE_DEMAND_DEFAULT_NM; 
    mcSettings.MaxTorqueDemand = MAX_MOTOR_TORQUE_NM;
    mcSettings.BrakeRegenTorqueDemand = MAX_REGEN_TORQUE_DEMAND_DEFAULT_NM;
    mcSettings.MaxRegenTorqueDemand = MAX_REGEN_TORQUE_NM;
    mcSettings.DirectionCommand = INVERTER_DIRECTION_FORWARD;

    // TODO - legacy settings - do we still need?
    mcSettings.ForwardSpeedLimit = SPEED_LIMIT_DEFAULT;
    mcSettings.DischargeCurrentLimit = DISCHARGE_CURRENT_LIMIT_DEFAULT;
    mcSettings.ChargeCurrentLimit = CHARGE_CURRENT_LIMIT_DEFAULT;
    return HAL_OK;
}

HAL_StatusTypeDef setMotorControllerSettings(MotorControllerSettings settings)
{
    memcpy(&mcSettings, &settings, sizeof(MotorControllerSettings));
    return HAL_OK;
}

HAL_StatusTypeDef setForwardSpeedLimit(float limit)
{
    mcSettings.ForwardSpeedLimit = limit;
    return HAL_OK;
}

HAL_StatusTypeDef setTorqueLimit(float limit)
{
    mcSettings.DriveTorqueLimit = limit;
    return HAL_OK;
}

HAL_StatusTypeDef setRegenTorqueLimit(float limit)
{
    mcSettings.MaxRegenTorqueDemand = limit;
    return HAL_OK;
}

HAL_StatusTypeDef setDischargeCurrentLimit(float limit)
{
    mcSettings.DischargeCurrentLimit = limit;
	return HAL_OK;
}

HAL_StatusTypeDef mcReadParamCommand(uint16_t address, uint16_t data) {
    VCU_INV_Parameter_RW_Command = INVERTER_PARAM_READ;
    VCU_INV_Parameter_Address = address;
    VCU_INV_Parameter_Data = data; // Is this needed for reading? todo

    if (sendCAN_MC_Read_Write_Param_Command() != HAL_OK) {
        ERROR_PRINT("Failed to send read param message to MC\n");
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef mcWriteParamCommand(uint16_t address, uint16_t data) {
    VCU_INV_Parameter_RW_Command = INVERTER_PARAM_WRITE;
    VCU_INV_Parameter_Address = address;
    VCU_INV_Parameter_Data = data;

    if (sendCAN_MC_Read_Write_Param_Command() != HAL_OK) {
        ERROR_PRINT("Failed to send write param message to MC\n");
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef mcClearFaults() {
    return mcWriteParamCommand(INVERTER_FAULT_CLEAR_ADDRESS, 0);
}

HAL_StatusTypeDef mcInit() {
    DEBUG_PRINT("Waiting for MC to complete startup checks\n");
    // Todo - Read and check EEPROM values

    DEBUG_PRINT("Starting MC ..\n");

    uint32_t startTick = xTaskGetTickCount();

    // Attempt to disable the lockout
    while ((xTaskGetTickCount() - startTick < (INVERTER_ON_TIMEOUT_MS)) && 
           (isLockoutDisabled() == false))
    {
        sendLockoutReleaseToMC();
        vTaskDelay(pdMS_TO_TICKS(THROTTLE_POLL_TIME_MS));
    }


    if ((isLockoutDisabled() == false) ||
        (getInverterVSMState() == INV_VSM_State_FAULT_STATE)) {
        ERROR_PRINT("Inverter lockout could not be released\n");
        return HAL_TIMEOUT;
    }

    DEBUG_PRINT("Motor controller ready.\n");

    // This isn't really used, but may have future use?
    initMotorControllerSettings();

    requestTorqueFromMC(0, MOTORING);

    return HAL_OK;
}

HAL_StatusTypeDef sendDisableMC(void) {
    VCU_INV_Inverter_Enable = INVERTER_OFF;
    VCU_INV_Inverter_Discharge = INVERTER_DISCHARGE_DISABLE;
    VCU_INV_Speed_Mode_Enable = SPEED_MODE_OVERRIDE_FALSE;
    VCU_INV_Torque_Limit_Command = TORQUE_LIMIT_OVERRIDE_FALSE;

    if (sendCAN_MC_Command_Message() != HAL_OK) {
        ERROR_PRINT("Failed to send disable message to MC\n");
        sendDTC_FATAL_VCU_F7_MC_DISABLE_ERROR();
        return HAL_ERROR;
    }

    return HAL_OK;
}


HAL_StatusTypeDef sendLockoutReleaseToMC() {
    // Based on Cascadia Motion documentation, need to send an inverter disable command to release lockout
    // Note - lockout will not disable if inverter is faulted

    return sendDisableMC();
}

/**
 * @brief Maps the positive throttle percent to a motoring torque demand
 */
float mapThrottleToTorque(float throttle_percent) {
    if (throttle_percent < MIN_THROTTLE_PERCENT_FOR_TORQUE) {
        throttle_percent = 0.0f;
    }
   
    float maxTorqueDemand = min(mcSettings.MaxTorqueDemand, mcSettings.DriveTorqueLimit);
    #ifdef ENABLE_POWER_LIMIT
    maxTorqueDemand = min(maxTorqueDemand, max(0, INV_POWER_LIMIT/(INV_Motor_Speed*RPM_TO_RAD))); // P=Tω 
    #endif

    float scaledTorque = map_range_float(throttle_percent, MIN_THROTTLE_PERCENT_FOR_TORQUE, 100, 0, maxTorqueDemand);
    return scaledTorque; 
}

/**
 * @brief Maps the positive brake percent to a regen torque demand
 * @returns the regen torque in Nm (this is a positive value), see requestTorqueFromMC for details
 */
float mapBrakeToRegenTorque(float brake_percent) {
    float rearBrakePres = RearBrakePressure;
    float frontBrakePres = FrontBrakePressure;
    float maxRegenTorqueDemand = min(mcSettings.MaxRegenTorqueDemand, mcSettings.BrakeRegenTorqueDemand);
    // if we have low brake pressure, work of off the brake angle as it gets very jittery otherwise
    // if (rearBrakePres < MIN_REGEN_REAR_BRAKE_PRESSURE) {
    //     float scaledTorque = map_range_float(brake_percent, MIN_BRAKE_PERCENT_FOR_REGEN_TORQUE, 100, 0, 0.7*maxRegenTorqueDemand);
    //     return scaledTorque;
    // }
    // (rear + psi_reqd) / (rear + psi_reqd + front) = pct
    // (rear + psi_reqd)  = pct * (rear + psi_reqd + front)
    // (1-pct)*(rear + psi_reqd) = pct * front
    // rear + psi_reqd = (pct * front) / (1-pct)
    // psi_reqd = (pct * front) / (1-pct) - rear
    float psi_reqd = DESIRED_REAR_BRAKE_BIAS_PCT * frontBrakePres / (1 - DESIRED_REAR_BRAKE_BIAS_PCT) - rearBrakePres;
    // from jeremy: Brake torque per wheel [Nm} = Pressure in circuit [Pa] * 4.489x10^-5 [m^3]
    float Pa_reqd = psi_reqd * PSI_2_PA; // psi to Pa
    float torque_reqd = Pa_reqd * 4.489e-5f; // m^3 to Nm
    // DEBUG_PRINT("T: %.3f, P: %.0f, F: %.0f, R: %.0f, psi_reqd: %.0f\n", torque_reqd, Pa_reqd, frontBrakePres, rearBrakePres, psi_reqd);
    
    if (torque_reqd > maxRegenTorqueDemand) {
        torque_reqd = maxRegenTorqueDemand;
    }
    if (torque_reqd < 0) {
        torque_reqd = 0;
    }
    return torque_reqd;
}

/**
 * @brief Requests positive or negative torque from the motor controller
 * @param requestTorque The requested torque in Nm (this must be positive)
 * @param commandMode The command mode (MOTORING or REGEN)
 */
HAL_StatusTypeDef requestTorqueFromMC(float requestTorque, InvCommandMode_t commandMode) {
    if (requestTorque < 0) {
        ERROR_PRINT("Requested torque must be positive, to command regen torque set commandMode to REGEN\n");
        return HAL_ERROR;
    }

    float maxTorqueDemand = 0.1; // Cannot be 0 as this would default to the values in the CM200DZ eeprom. see page 89 of https://www.cascadiamotion.com/uploads/5/1/3/0/51309945/0a-0163-02_sw_user_manual.pdf
    switch (commandMode) {
        case MOTORING:
            requestTorque = requestTorque;
            maxTorqueDemand = min(mcSettings.MaxTorqueDemand, mcSettings.DriveTorqueLimit);
            #ifdef ENABLE_POWER_LIMIT
            maxTorqueDemand = min(maxTorqueDemand, INV_POWER_LIMIT/(INV_Motor_Speed*RPM_TO_RAD)); // P=Tω 
            #endif
            break;
        case REGEN:
            requestTorque = -requestTorque;
            maxTorqueDemand = min(mcSettings.MaxRegenTorqueDemand, mcSettings.BrakeRegenTorqueDemand);
            break;
        default:
            ERROR_PRINT("Invalid command mode\n");
            return HAL_ERROR;
    }

    INV_Tractive_Power_kW = INV_DC_Bus_Voltage*INV_DC_Bus_Current*W_TO_KW; //V and I values are sent as V*10 and A*10
    sendCAN_VCU_INV_Power();

    VCU_INV_Torque_Command = requestTorque*INV_TORQUE_SCALING_FACTOR;
    VCU_INV_Speed_Command = TORQUE_MODE_SPEED_REQUEST;
    VCU_INV_Direction_Command = INVERTER_DIRECTION_FORWARD;
    VCU_INV_Inverter_Enable = INVERTER_ON;
    VCU_INV_Inverter_Discharge = INVERTER_DISCHARGE_DISABLE;
    VCU_INV_Speed_Mode_Enable = SPEED_MODE_OVERRIDE_FALSE;
    VCU_INV_Torque_Limit_Command = 0;

    if (sendCAN_MC_Command_Message() != HAL_OK) {
        ERROR_PRINT("Failed to send command message to MC\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}
