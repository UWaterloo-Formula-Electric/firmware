#include "motorController.h"
#include "bsp.h"
#include "vcu_F7_can.h"
#include "debug.h"
#include "task.h"
#include "string.h"
#include "inttypes.h"
#include "drive_by_wire.h"
#include "vcu_F7_dtc.h"
#include "mathUtils.h"

// MC Questions:
// Do we need to wait to close contactors until MCs are ready?
// How to read IDs for msgs on datasheet?

MotorControllerSettings mcSettings = {0};

// This is bad but a band-aid, we should really create a good way and check inverter status
volatile bool motors_active = false;

HAL_StatusTypeDef initMotorControllerSettings()
{
    mcSettings.InverterMode = 0;
    mcSettings.DriveTorqueLimit = MAX_TORQUE_DEMAND_DEFAULT_NM; 
    mcSettings.MaxTorqueDemand = MAX_MOTOR_TORQUE_NM;
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
        // mcClearFaults();
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

    requestTorqueFromMC(0);
    motors_active = true;

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

HAL_StatusTypeDef mcDisable() {   
	motors_active = false;
	// Wait for final throttle messages to exit our CAN queue
    vTaskDelay(pdMS_TO_TICKS(250));
    
    return sendDisableMC();
}


HAL_StatusTypeDef sendLockoutReleaseToMC() {
    // Based on Cascadia Motion documentation, need to send an inverter disable command to release lockout
    // Note - lockout will not disable if inverter is faulted

    if (isLockoutDisabled()) {
        // Don't need to release
        return HAL_OK;
    }

    return sendDisableMC();
}


HAL_StatusTypeDef requestTorqueFromMC(float throttle_percent) {

    if (throttle_percent < MIN_THROTTLE_PERCENT_FOR_TORQUE)
    {
        throttle_percent = 0.0f;
    }
    // Per Cascadia Motion docs, torque requests are sent in Nm * 10
    float maxTorqueDemand = min(mcSettings.MaxTorqueDemand, mcSettings.DriveTorqueLimit);
    float scaledTorque = map_range_float(throttle_percent, MIN_THROTTLE_PERCENT_FOR_TORQUE, 100, 0, maxTorqueDemand);
    uint16_t requestTorque = scaledTorque; 

    VCU_INV_Torque_Command = requestTorque*10;
    VCU_INV_Speed_Command = TORQUE_MODE_SPEED_REQUEST;
    VCU_INV_Direction_Command = INVERTER_DIRECTION_FORWARD;
    VCU_INV_Inverter_Enable = INVERTER_ON;
    VCU_INV_Inverter_Discharge = INVERTER_DISCHARGE_DISABLE;
    VCU_INV_Speed_Mode_Enable = SPEED_MODE_OVERRIDE_FALSE;
    VCU_INV_Torque_Limit_Command = TORQUE_LIMIT_OVERRIDE_FALSE;

    if (sendCAN_MC_Command_Message() != HAL_OK) {
        ERROR_PRINT("Failed to send command message to MC\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}
