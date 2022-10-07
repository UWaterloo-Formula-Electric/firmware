/**
  ****************************************************************************
  * @file    controlStateMachine.h
  * @author  Richard Matthews
  * @brief   State machine controlling BMU's high level logic
  * @details State machine logic, including transition array, and transition
  *          function. 
  ****************************************************************************
  */

#ifndef CONTROLSTATEMACHINE_H

#define CONTROLSTATEMACHINE_H

#include "stm32f7xx_hal.h"
#include "state_machine.h"

typedef enum BMU_States_t {
    STATE_Self_Check = 0,		 ///< 0: Self check performed on boot
    STATE_Wait_System_Up,		 ///< 1: Waiting for the IMD and Fault Monitor
    STATE_HV_Disable,			 ///< 2: Battery pack contactors open
    STATE_HV_Enable,			 ///< 3: Battery pack contactors closed
    STATE_Precharge,			 ///< 4: Waiting for precharge to complete
    STATE_Discharge,			 ///< 5: Waiting for discharge to complete
    STATE_Charging,				 ///< 6: Battery is currently being charged
    STATE_Failure_Fatal,		 ///< 7: System encountered a critical failture
    STATE_Failure_CBRB_Disabled, ///< 8: Cockpit BRB has been pressed when not at HV, non critical error
    STATE_Failure_CBRB_Enabled,  ///< 9: Cockpit BRB has been pressed when at HV, non critical error 
    STATE_Balancing,             ///< 10: The cells are being balanced
    STATE_ANY,					 ///< 11: Must be the last state
} BMU_States_t;

char BMU_states_string[][25] =
{
    "Self check", 
    "Wait System Up", 
    "HV Disable",
    "HV Enable", 
    "Precharge", 
    "Discharge",
    "Charging", 
    "Failure fatal", 
    "Failure CBRB Disabled",
    "Failure CBRB Enabled",
    "Balancing"
};

typedef enum BMU_Events_t {
    EV_Init = 0,                ///< 0: Event to init the state machine
    EV_HV_Toggle,               ///< 1: Triggered by CAN message from DCU
    EV_Precharge_Finished,      ///< 2: Precharge is complete
    EV_Discharge_Finished,      ///< 3: Discharge is complete
    EV_PrechargeDischarge_Fail, ///< 4: PC/DC has failed, check debug log
    EV_HV_Fault,                ///< 5: HV needs to be shutdown immediately
    EV_IMD_Ready,               ///< 6: IMD is ready, checked on start up
    EV_FaultMonitorReady,       ///< 7: Safety IL is closed and ready
    EV_Enter_Charge_Mode,       ///< 8: Charge mode triggered over CAN
    EV_Charge_Start,            ///< 9: Charging begins, triggered over CAN
    EV_Notification_Done,       ///< 10: Charging/Cell Balancing is done
    EV_Charge_Error,            ///< 11: Error during balance charging
    EV_Notification_Stop,       ///< 12: Stopped charging/balancing. This is sent after the batteryTask returns from balanceCharge()
    EV_Cockpit_BRB_Pressed,     ///< 13: Cockpit BRB has been pressed
    EV_Cockpit_BRB_Unpressed,   ///< 14: Cockpit BRB has been released
    EV_Balance_Start,           ///< 15: Start balancing the cells
    EV_Balance_Stop,            ///< 16: Request stop cell balancing. Only called by CLI command
    EV_ANY,                     ///< 17: Must be the last event
} BMU_Events_t;

typedef enum BMU_SystemUpFail {
    BMU_NO_FAIL,
    IMD_FAIL,
    IL_HVIL_FAIL,
    UNKOWN_FAIL,
} BMU_SystemUpFail;

extern FSM_Handle_Struct fsmHandle;

HAL_StatusTypeDef controlInit(void);
HAL_StatusTypeDef startControl();
void setChargeMode(bool charge_mode);
void controlTask(void *pvParameters);

#endif /* end of include guard: CONTROLSTATEMACHINE_H */
