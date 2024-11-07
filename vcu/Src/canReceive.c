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

void CAN_Msg_DCU_buttonEvents_Callback()
{
    DEBUG_PRINT_ISR("Received DCU button Event\n");
    if (ButtonEMEnabled) {
        fsmSendEventISR(&fsmHandle, EV_EM_Toggle);
    }
    else if(ButtonEnduranceToggleEnabled) 
    {
		toggle_endurance_mode();
	}
	else if(ButtonEnduranceLapEnabled)
	{
		trigger_lap();
	}
	else if(ButtonTCEnabled)
	{
		toggle_TC();
	}
}

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
    fsmSendEventUrgentISR(&fsmHandle, EV_Fatal);
}

void CAN_Msg_BMU_HV_Power_State_Callback() {
    DEBUG_PRINT_ISR("Receive hv power state\n");
    if (HV_Power_State != HV_Power_State_On) {
        fsmSendEventISR(&fsmHandle, EV_Hv_Disable);
        resetMCLockout();
    }
}

void CAN_Msg_BMU_DTC_Callback(int DTC_CODE, int DTC_Severity, int DTC_Data) {
    switch (DTC_CODE) {
        case WARNING_CONTACTOR_OPEN_IMPENDING:
            fsmSendEventISR(&fsmHandle, EV_Hv_Disable);
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