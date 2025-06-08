/**
 *******************************************************************************
 * @file    canReceive.c
 * @author	Richard
 * @date    Dec 2024
 * @brief   Contains CAN callback functions and functions to obtain CAN signals.
 *          Receives IMD and AMS Faults for IMD and AMS fault lights
 *
 ******************************************************************************
 */
#include "canReceive.h"

#include "userCan.h"
#include "bsp.h"
#include "debug.h"

#include "drive_by_wire.h"
#include "motorController.h"

#include "vcu_F7_can.h"
#include "vcu_F7_dtc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#include "endurance_mode.h"
#include "traction_control.h"
#include "motorController.h"

/*
 * External Board Statuses:
 * Variables for keeping track of external board statuses that get updated by
 * Can messages
 */
volatile bool motorControllersStatus = false;
volatile bool inverterLockoutDisabled = false;
volatile uint8_t inverterVSMState;
volatile uint8_t inverterInternalState;

/*
 * Functions to get external board status
 */
bool getHvEnableState()
{
    return HV_Power_State == HV_Power_State_On;
}

bool getMotorControllersStatus()
{
    return motorControllersStatus;
}

bool isLockoutDisabled()
{
    return inverterLockoutDisabled;
}

void resetMCLockout()
{
    inverterLockoutDisabled = false;
}

uint8_t getInverterVSMState()
{
    return inverterVSMState;
}

extern osThreadId driveByWireHandle;


// TODO: remove. This is happening locally now
// void CAN_Msg_VCU_buttonEvents_Callback()
// {
//     DEBUG_PRINT_ISR("Received button event\n");
//     if (ButtonEMEnabled) {
//         fsmSendEventISR(&VCUFsmHandle, EV_EM_Toggle);
//     }
//     else if(ButtonEnduranceToggleEnabled) 
//     {
// 		toggle_endurance_mode();
// 	}
// 	else if(ButtonEnduranceLapEnabled)
// 	{
// 		trigger_lap();
// 	}
// 	else if(ButtonTCEnabled)
// 	{
// 		toggle_TC();
// 	}
// }

void CAN_Msg_PDU_ChannelStatus_Callback()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (!motorControllersStatus && StatusPowerInverter == StatusPowerInverter_CHANNEL_ON) {
        xTaskNotifyFromISR( driveByWireHandle,
                            (1<<NTFY_MCs_ON),
                            eSetBits,
                            &xHigherPriorityTaskWoken );
        motorControllersStatus = true;
    } else if (motorControllersStatus) {
        // Only send a notification if MCs turned off if MCs were already ON
        xTaskNotifyFromISR( driveByWireHandle,
                            (1<<NTFY_MCs_OFF),
                            eSetBits,
                            &xHigherPriorityTaskWoken );
        motorControllersStatus = false;
    }

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void DTC_Fatal_Callback(BoardIDs board)
{
    DEBUG_PRINT_ISR("DTC Receieved from board %lu \n", board);
    fsmSendEventUrgentISR(&VCUFsmHandle, EV_Fatal);
}

/**
 * Precharge state CAN message callback.
 * Toggles the HV led when precharging
 */
void CAN_Msg_PrechargeState_Callback() {
    DEBUG_PRINT_ISR("Received precharge state %u\n", (uint8_t)PrechargeState);
    if (PrechargeState < 5){
        setPendingHvResponse();
    }
}
/**
 * BMU Power state CAN message callback.
 * Signals to the main task the HV power state has changed
 */
void CAN_Msg_BMU_HV_Power_State_Callback() {
    DEBUG_PRINT_ISR("Receive hv power state\n");
    receivedHvResponse();
    if (HV_Power_State != HV_Power_State_On) {
        fsmSendEventISR(&VCUFsmHandle, EV_Hv_Disable);
        resetMCLockout();
    } else {
        fsmSendEventISR(&VCUFsmHandle, EV_CAN_Receive_HV);
    }
}

void CAN_Msg_BMU_DTC_Callback(int DTC_CODE, int DTC_Severity, int DTC_Data) {
    switch (DTC_CODE) {
        case WARNING_CONTACTOR_OPEN_IMPENDING:
            fsmSendEventISR(&VCUFsmHandle, EV_Hv_Disable);
            break;
        case FATAL_IMD_Failure:
            ERROR_PRINT_ISR("Got IMD failure\n");
            IMD_FAIL_LED_ON
            break;
        case FATAL_AMS_Failure:
        case CRITICAL_CELL_VOLTAGE_LOW: // All 4 cases fall through
        case CRITICAL_CELL_VOLTAGE_HIGH:
        case CRITICAL_CELL_TEMP_HIGH:
        case ERROR_CELL_TEMP_LOW:
            ERROR_PRINT_ISR("Got AMS failure\n");
            AMS_FAIL_LED_ON
            break;
        default:
            // Do nothing, other events handled by fatal callback
            break;
    }
}


void CAN_Msg_UartOverCanConfig_Callback()
{
    isUartOverCanEnabled = UartOverCanConfigSignal & 0x1;
}


void CAN_Msg_TractionControlConfig_Callback()
{
	// This is pretty terrible and should be wrapped in function calls or like have changed names
	tc_kP = TC_kP;
	tc_kI = TC_kI;
	tc_kD = TC_kD;
	desired_slip = TC_desiredSlipPercent;
	DEBUG_PRINT_ISR("tc_kP is %f\n", tc_kP);
}

void CAN_Msg_MC_Internal_States_Callback() // 100 hz
{
    inverterLockoutDisabled = INV_Inverter_Enable_Lockout == 0;
    inverterInternalState = INV_Inverter_State;
    inverterVSMState = INV_VSM_State;
} 

void CAN_Msg_MC_Read_Write_Param_Response_Callback()
{
    sendLockoutReleaseToMC();
}

void CAN_Msg_MC_Current_Info_Callback(void)
{
    const float instPowerKw = (INV_DC_Bus_Current * INV_DC_Bus_Voltage) * W_TO_KW;
    if (instPowerKw > INV_Peak_Tractive_Power_kW)
    {
        INV_Peak_Tractive_Power_kW = instPowerKw;
    }
}

/* PORTED FROM THE DCU!!! */
/**
 * Get current HV power state, updated from BMU CAN messages
 */
bool getHVState()
{
    return HV_Power_State;
}

/**
 * Get current EM enabled state, updated from VCU CAN messages
 */
bool getEMState()
{
    return EM_State;
}