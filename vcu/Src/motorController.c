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

//comment out to remove 80kw power limit
#define ENABLE_POWER_LIMIT
#define INV_POWER_LIMIT 70000.0 //80kw
#define RPM_TO_RAD (2.0*3.14159/60.0)

MotorControllerSettings mcSettings = {0};

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

HAL_StatusTypeDef mcReadParamCommand(uint16_t address, int *data) {
    VCU_INV_Parameter_RW_Command = INVERTER_PARAM_READ;
    VCU_INV_Parameter_Address = address;
    VCU_INV_Parameter_Data = 0; // Is this needed for reading? todo

    if (sendCAN_MC_Read_Write_Param_Command() != HAL_OK) {
        ERROR_PRINT("Failed to send read param message to MC\n");
        return HAL_ERROR;
    }

    if(INV_Parameter_Response_Addr != address) {
        DEBUG_PRINT("INV response address does not match");
        return HAL_ERROR;
    }

    *data = INV_Parameter_Response_Data;
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

    requestTorqueFromMC(0);

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


HAL_StatusTypeDef requestTorqueFromMC(float throttle_percent) {

    if (throttle_percent < MIN_THROTTLE_PERCENT_FOR_TORQUE)
    {
        throttle_percent = 0.0f;
    }
    // Per Cascadia Motion docs, torque requests are sent in Nm * 10
    float maxTorqueDemand = min(mcSettings.MaxTorqueDemand, mcSettings.DriveTorqueLimit);
    #ifdef ENABLE_POWER_LIMIT
    maxTorqueDemand = min(maxTorqueDemand, INV_POWER_LIMIT/(INV_Motor_Speed*RPM_TO_RAD)); // P=Tω 
    #endif

    INV_Tractive_Power_kW = (INV_DC_Bus_Voltage*INV_DC_Bus_Current)*W_TO_KW; //V and I values are sent as V*10 and A*10
    sendCAN_VCU_INV_Power();

    float scaledTorque = map_range_float(throttle_percent, MIN_THROTTLE_PERCENT_FOR_TORQUE, 100, 0, maxTorqueDemand);
    uint16_t requestTorque = scaledTorque; 

    VCU_INV_Torque_Command = requestTorque*10;
    VCU_INV_Speed_Command = TORQUE_MODE_SPEED_REQUEST;
    VCU_INV_Direction_Command = INVERTER_DIRECTION_FORWARD;
    VCU_INV_Inverter_Enable = INVERTER_ON;
    VCU_INV_Inverter_Discharge = INVERTER_DISCHARGE_DISABLE;
    VCU_INV_Speed_Mode_Enable = SPEED_MODE_OVERRIDE_FALSE;
    VCU_INV_Torque_Limit_Command = maxTorqueDemand;

    if (sendCAN_MC_Command_Message() != HAL_OK) {
        ERROR_PRINT("Failed to send command message to MC\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}
