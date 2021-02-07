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
    STATE_Self_Check = 0, ///< 0: Self check performed on boot
    STATE_Wait_System_Up, ///< 1: Waiting for the IMD and Fault Monitor
    STATE_HV_Disable,     ///< 2: Battery pack contactors open
    STATE_HV_Enable,      ///< 3: Battery pack contactors closed
    STATE_Precharge,      ///< 4: Waiting for precharge to complete
    STATE_Discharge,      ///< 5: Waiting for discharge to complete
    STATE_Charging,       ///< 6: Battery is currently being charged
    STATE_Failure_Fatal,  ///< 7: System encountered a critical failture
    STATE_ANY,            ///< 8: Must be the last state
} BMU_States_t;

/*! \var BMU_States_t STATE_Self_Check
 *  State machine begins in this state. Currently we don't have any self tests
 *  that run, but this is an area for improvement.
 */

/*! \var BMU_States_t STATE_Wait_System_Up
 *  The BMU stays in this state until receiving ::EV_IMD_Ready and
 *  ::EV_FaultMonitorReady.
 *  See @ref systemUpCheck and @ref faultMonitorTask for more details.
 */

/*! \var BMU_States_t STATE_Wait_System_Up
 *  The BMU stays in this state until receiving ::EV_IMD_Ready and
 *  ::EV_FaultMonitorReady.
 *  See @ref systemUpCheck and @ref faultMonitorTask for more details.
 */

typedef enum BMU_Events_t {
    EV_Init = 0,
    EV_HV_Toggle,
    EV_Precharge_Finished,
    EV_Discharge_Finished,
    EV_PrechargeDischarge_Fail,
    EV_HV_Fault,
    EV_IMD_Ready,
    EV_FaultMonitorReady,
    EV_Enter_Charge_Mode,
    EV_Charge_Start,
    EV_Charge_Done,
    EV_Charge_Error,
    EV_Charge_Stop,
    EV_ANY, // Must be the last event
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
void controlTask(void *pvParameters);

#endif /* end of include guard: CONTROLSTATEMACHINE_H */
