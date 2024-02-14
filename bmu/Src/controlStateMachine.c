/**
  *****************************************************************************
  * @file    controlStateMachine.c
  * @author  Richard Matthews
  * @brief   State machine controlling BMU's high level logic
  * @details State machine logic, including transition array, and transition
  * function. See confluence for more details
  ******************************************************************************
  */

#include "controlStateMachine.h"
#include "stm32f7xx_hal.h"
#include "stdbool.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "debug.h"
#include "state_machine.h"
#include "bmu_dtc.h"
#include "bmu_can.h"
#include "prechargeDischarge.h"
#include "bsp.h"
#include "watchdog.h"
#include "batteries.h"
#include "chargerControl.h"

extern osThreadId PCDCHandle;
extern osThreadId BatteryTaskHandle;

FSM_Handle_Struct fsmHandle;

bool gChargeMode;

uint32_t runSelftTests(uint32_t event);
uint32_t startPrecharge(uint32_t event);
uint32_t startDischarge(uint32_t event);
uint32_t dischargeFinished(uint32_t event);
uint32_t prechargeFinished(uint32_t event);
uint32_t controlDoNothing(uint32_t event);
uint32_t DefaultTransition(uint32_t event);
uint32_t handleFault(uint32_t event);
uint32_t stopPrecharge(uint32_t event);
uint32_t enterChargeMode(uint32_t event);
uint32_t startCharge(uint32_t event);
uint32_t stopCharge(uint32_t event);
uint32_t chargeDone(uint32_t event);
uint32_t systemUpCheck(uint32_t event);
uint32_t systemNotReady(uint32_t event);
uint32_t cockpitBRBPressed(uint32_t event);
uint32_t cockpitBRBReleased(uint32_t event);
uint32_t startBalance(uint32_t event);
uint32_t balanceStopRequest(uint32_t event);
uint32_t stopBalance(uint32_t event);
uint32_t balanceDone(uint32_t event);

Transition_t transitions[] = {
    { STATE_Self_Check, EV_Init, &runSelftTests },
    { STATE_Wait_System_Up, EV_IMD_Ready, &systemUpCheck },
    { STATE_Wait_System_Up, EV_FaultMonitorReady, &systemUpCheck },
    { STATE_Wait_System_Up, EV_ANY, &systemNotReady },

    //Balance
    { STATE_HV_Disable, EV_Balance_Start, &startBalance }, // A request was made via the CLI to start cell balancing
    { STATE_Balancing,  EV_Balance_Stop, &balanceStopRequest }, //A request was made via the CLI to request stop. Once the battery task exits the balancing loop it will send an EV_Notification_Stop
    { STATE_Balancing,  EV_Notification_Stop, &stopBalance }, // Cell balancing was stopped and battery task is back to normal
    { STATE_Balancing, EV_Notification_Done, &balanceDone }, // Cell balancing is done and battery task is back to normal

    // Charge
    { STATE_HV_Disable, EV_Enter_Charge_Mode, &enterChargeMode },
    { STATE_HV_Enable, EV_Charge_Start, &startCharge },
    { STATE_Charging, EV_Notification_Stop, &stopCharge },
    { STATE_Charging, EV_Notification_Done, &chargeDone }, // Takes precedence over next transition
    { STATE_ANY, EV_Notification_Stop, &controlDoNothing }, // Must be after charging charge stop

    // PCDC
    { STATE_HV_Disable, EV_HV_Toggle, &startPrecharge },
    { STATE_Precharge, EV_Precharge_Finished, &prechargeFinished },
    { STATE_HV_Enable, EV_HV_Toggle, &startDischarge },
    { STATE_Precharge, EV_HV_Toggle, &stopPrecharge },
    { STATE_Discharge, EV_Discharge_Finished, &dischargeFinished },
    { STATE_Discharge, EV_HV_Toggle, &controlDoNothing},

    // When not hv disabled, ignore charge mode message
    { STATE_ANY, EV_Enter_Charge_Mode, &controlDoNothing },

	// Cockpit BRB pressed/unpressed
	{ STATE_ANY, EV_Cockpit_BRB_Pressed, &cockpitBRBPressed },
	{ STATE_Failure_CBRB_Disabled, EV_Cockpit_BRB_Unpressed, &cockpitBRBReleased},
	{ STATE_Failure_CBRB_Enabled, EV_Cockpit_BRB_Unpressed, &startPrecharge},
    { STATE_Failure_CBRB_Enabled, EV_Discharge_Finished, &dischargeFinished },

    // Already in failure, do nothing
    // Takes priority over rest of events
    { STATE_Failure_Fatal, EV_ANY, &controlDoNothing },
    { STATE_ANY, EV_HV_Fault, &handleFault},
    { STATE_ANY, EV_PrechargeDischarge_Fail, &handleFault },
    { STATE_ANY, EV_Charge_Error, &handleFault },
    { STATE_ANY, EV_ANY, &DefaultTransition}
};

HAL_StatusTypeDef controlInit()
{
    FSM_Init_Struct init;

    init.maxStateNum = STATE_ANY;
    init.maxEventNum = EV_ANY;
    init.sizeofEventEnumType = sizeof(BMU_Events_t);
    init.ST_ANY = STATE_ANY;
    init.EV_ANY = EV_ANY;
    init.transitions = transitions;
    init.transitionTableLength = TRANS_COUNT(transitions);
    init.eventQueueLength = 5;
    init.watchdogTaskId = 1;
    if (fsmInit(STATE_Self_Check, &init, &fsmHandle) != HAL_OK) {
        ERROR_PRINT("Control fsm init Failed\n");  //Failed to init control fsm
        return HAL_ERROR;
    }

    if (registerTaskToWatch(1, 100, true, &fsmHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    DEBUG_PRINT("Init control fsm\n");
    return HAL_OK;
}

/**
 * Task to manage the state machine.
 */
void controlTask(void *pvParameters)
{
    // Pre send EV_INIT to kick off self tests
    startControl();

    if (canStart(&CAN_HANDLE) != HAL_OK) {
        Error_Handler();
    }

    fsmTaskFunction(&fsmHandle);

    for(;;); // Shouldn't reach here
}

HAL_StatusTypeDef startControl()
{
    return fsmSendEvent(&fsmHandle, EV_Init, portMAX_DELAY /* timeout */); // Force run of self checks
}


/// Bool to record receiving the event that the IMD is ready
bool imdReady = false;
/// Bool to record recieving the event that the Fault Monitor is ready
bool faultMonitorReady = false;

/**
 * @brief Checks if the system is ready to run
 * Checks components of the IL to make sure the system is ready to run
 * @return @BMU_SystemUpFail
 */
BMU_SystemUpFail isSystemReady()
{
    if (imdReady && faultMonitorReady) {
        return BMU_NO_FAIL;
    } else if (!imdReady) {
        return IMD_FAIL;
    } else if (!faultMonitorReady) {
        return IL_HVIL_FAIL;
    } else {
        return UNKOWN_FAIL;
    }
}

uint32_t systemUpCheck(uint32_t event)
{
    if (event == EV_IMD_Ready) {
        DEBUG_PRINT("IMD Ready\n");
        imdReady = true;
    }

    if (event == EV_FaultMonitorReady) {
        DEBUG_PRINT("Fault monitor ready\n");
        faultMonitorReady = true;
    }

    // Check all ready to start conditions
    if (isSystemReady() == BMU_NO_FAIL) {
        DEBUG_PRINT("System up!\n");
        return STATE_HV_Disable;
    } else {
        return STATE_Wait_System_Up;
    }
}

uint32_t runSelftTests(uint32_t event)
{
    // TODO: Run some tests
    // Example of tests to run:
    // - Check the AMS boards are functioning correctly:
    //   - DIAGN command to test MUX functionality
    //   - CVST, AXST, STATST commands to test digital filters and memory
    //   - ADAXD to check for differences between ADCs
    //   - ADSTAT to ensure VREG and VREGD are within acceptable levels
    //   - ADOL to compare ADCs
    //   - AXOW for an auxillary open wire check

    DEBUG_PRINT("Tests done awaiting system\n");    //Self tests done, wating for system to come up

    // On startup, we aren't in charge mode until otherwise notified
    gChargeMode = false;

    return STATE_Wait_System_Up;
}

uint32_t controlDoNothing(uint32_t event)
{
    return fsmGetState(&fsmHandle);
}

uint32_t DefaultTransition(uint32_t event)
{
    ERROR_PRINT("No trans func - %lu, %lu\n",      //No transition function registered for state %lu, event %lu\n
                fsmGetState(&fsmHandle), event);

    sendDTC_WARNING_BMU_UNKOWN_EVENT_STATE_COMBO(event);
    return fsmGetState(&fsmHandle);
}

uint32_t startPrecharge(uint32_t event)
{
    DEBUG_PRINT("starting precharge\n");

    if (gChargeMode) {
        xTaskNotify(PCDCHandle, (1<<PRECHARGE_NOTIFICATION_CHARGER), eSetBits);
    } else {
        xTaskNotify(PCDCHandle, (1<<PRECHARGE_NOTIFICATION_MOTOR_CONTROLLERS), eSetBits);
    }

    return STATE_Precharge;
}

uint32_t startDischarge(uint32_t event)
{
    DEBUG_PRINT("starting discharge\n");
    xTaskNotify(PCDCHandle, (1<<DISCHARGE_NOTIFICATION), eSetBits);
    return STATE_Discharge;
}

uint32_t dischargeFinished(uint32_t event)
{
	
    uint32_t currentState = fsmGetState(&fsmHandle);
    DEBUG_PRINT("discharge finished\n");

    HV_Power_State = HV_Power_State_Off;
    //sendCAN_BMU_HV_Power_State();

    DC_DC_OFF;
	if(currentState == STATE_Failure_CBRB_Enabled || currentState == STATE_Failure_CBRB_Disabled)
	{
		return currentState;
	}
    return STATE_HV_Disable;
}

uint32_t prechargeFinished(uint32_t event)
{
    DEBUG_PRINT("precharge finished\n");

    HV_Power_State = HV_Power_State_On;
    //sendCAN_BMU_HV_Power_State();

    DC_DC_ON;

    return STATE_HV_Enable;
}

uint32_t stopDischarge()
{
    DEBUG_PRINT("stopDischarge\n");
    xTaskNotify(PCDCHandle, (1<<STOP_NOTIFICATION), eSetBits);
    return STATE_HV_Enable;
}

void sendStopPrecharge()
{
    DEBUG_PRINT("sending stopPrecharge\n");    //send notification to stopPrecharge
    xTaskNotify(PCDCHandle, (1<<STOP_NOTIFICATION), eSetBits);
}

uint32_t handleFault(uint32_t event)
{
    sendDTC_FATAL_BMU_ERROR();
    ERROR_PRINT("SM received fault\n");      //State machine received fault

    uint32_t currentState = fsmGetState(&fsmHandle);

    HV_Power_State = HV_Power_State_Off;
    //sendCAN_BMU_HV_Power_State();

    DC_DC_OFF;

    switch (currentState) {
        case STATE_HV_Disable:
            {
                DEBUG_PRINT("HVdown flt, do nothing\n");     //HV disabled fault, do nothing
            }
            break;
        case STATE_HV_Enable:
            {
                DEBUG_PRINT("hvEnabledHVFault, discharging\n");      //hvEnabledHVFault, starting discharge
                xTaskNotify(PCDCHandle, (1<<DISCHARGE_NOTIFICATION), eSetBits);
                return STATE_Failure_Fatal;
            }
            break;
        case STATE_Precharge:
            {
                DEBUG_PRINT("hvEnabledHVFault during precharge\n");
                // Only send stop if the precharge hasn't already failed
                // Otherwise it's been stopped already
                if (event != EV_PrechargeDischarge_Fail) {
                    sendStopPrecharge();
                }
            }
            break;
        case STATE_Discharge:
            {
                DEBUG_PRINT("Flt, cont discharge\n");      //Fault during discharge. Attempting to continue discharge
            }
            break;
        default:
            {
                ERROR_PRINT("Oth State HV flt discharging\n");      //HV Fault during other state. Starting discharge
                xTaskNotify(PCDCHandle, (1<<DISCHARGE_NOTIFICATION), eSetBits);
            }
            break;
    }

    return STATE_Failure_Fatal;
}

uint32_t stopPrecharge(uint32_t event)
{
    DEBUG_PRINT("Stopping precharge\n");
    sendStopPrecharge();

    return STATE_HV_Disable;
}

//A start balance command was issued from the CLI
uint32_t startBalance(uint32_t event)
{
    DEBUG_PRINT("Starting cell balancing\n");
    xTaskNotify(BatteryTaskHandle, (1<<BALANCE_START_NOTIFICATION), eSetBits);
    return STATE_Balancing;
}

//A request was made via the CLI to request stop. Once the battery task exits the balancing loop it will send an EV_Notification_Stop
uint32_t balanceStopRequest(uint32_t event)
{
    DEBUG_PRINT("Sent request to stop cell balancing\n");
    xTaskNotify(BatteryTaskHandle, (1<<BATTERY_STOP_NOTIFICATION), eSetBits);

    // Stay in charging state until we receive charge done event
    return STATE_Balancing;
}

//Battery Task is out of balancing loop and battery task is back to normal
uint32_t stopBalance(uint32_t event)
{
    DEBUG_PRINT("Cell balancing stopped\n");
    return STATE_HV_Disable;
}

uint32_t balanceDone(uint32_t event)
{
    DEBUG_PRINT("Done balancing\n");
    return STATE_HV_Disable;
}

uint32_t enterChargeMode(uint32_t event)
{
    DEBUG_PRINT("Entering charge mode\n");

    // Disable the car heartbeat, as we aren't connected to the car
    disableHeartbeat();

    if (chargerInit() != HAL_OK) {
        ERROR_PRINT("Failed to init charger\n");
    }

    gChargeMode = true;

    return STATE_HV_Disable;
}

uint32_t startCharge(uint32_t event)
{
    if(!gChargeMode){
        DEBUG_PRINT("Not chargemode, now HV_ENABLE");       //Not in charging mode, returning to HV_ENABLE
        return STATE_HV_Enable;
    }
    DEBUG_PRINT("Starting charge\n");
    xTaskNotify(BatteryTaskHandle, (1<<CHARGE_START_NOTIFICATION), eSetBits);

    return STATE_Charging;

}
uint32_t stopCharge(uint32_t event)
{
    DEBUG_PRINT("Stopping charge\n");
    xTaskNotify(BatteryTaskHandle, (1<<BATTERY_STOP_NOTIFICATION), eSetBits);

    // Stay in charging state until we receive charge done event
    return STATE_Charging;
}

uint32_t chargeDone(uint32_t event)
{
    DEBUG_PRINT("Charge done/stopped\n");

    if (fsmGetState(&fsmHandle) == STATE_Charging) {
        return STATE_HV_Enable;
    } else {
        // Shouldn't happen
        ERROR_PRINT("Charg evnt done, no charging\n");        //Got charge done event, but wasn't charging
        return STATE_HV_Disable;
    }
}

uint32_t systemNotReady(uint32_t event)
{
    ERROR_PRINT("Waiting for sys ready\n");      //Still waiting for system to be ready
    sendDTC_WARNING_BMU_SystemNotReady(isSystemReady());

    return STATE_Wait_System_Up;
}

void setChargeMode(bool charge_mode)
{
	gChargeMode = charge_mode;
}

uint32_t cockpitBRBPressed(uint32_t event)
{

    uint32_t currentState = fsmGetState(&fsmHandle);
    
    HV_Power_State = HV_Power_State_Off;
    //sendCAN_BMU_HV_Power_State();

    DC_DC_OFF;

    switch (currentState) {
        case STATE_HV_Disable:
            {
                DEBUG_PRINT("Cockpit BRB at HV Disable\n");     //Cockpit BRB pressed during HV Disable phase, nothing done
                return STATE_Failure_CBRB_Disabled;
            }
            break;
        case STATE_HV_Enable:
            {
                DEBUG_PRINT("HVen CockpitBRB, discharg\n");     //Cockpit BRB pressed during HV Enable, starting discharge
                xTaskNotify(PCDCHandle, (1<<DISCHARGE_NOTIFICATION), eSetBits);
                return STATE_Failure_CBRB_Enabled;
            }
            break;
        case STATE_Precharge:
            {
                DEBUG_PRINT("Precharge CockpitBRB\n");      //Cockpit BRB pressed during precharge
				sendStopPrecharge();
				return STATE_Failure_CBRB_Enabled;
            }
            break;
        case STATE_Discharge:
            {
                DEBUG_PRINT("Discharge CockpitBRB, cont\n");    //Cockpit BRB pressed during discharge. Attempting to continue discharge
                return STATE_Failure_CBRB_Disabled;
            }
            break;
        default:
            {
                ERROR_PRINT("Other CockpitBRB, discharg\n");        //Cockpit BRB during other state. Starting discharge
                xTaskNotify(PCDCHandle, (1<<DISCHARGE_NOTIFICATION), eSetBits);
                return STATE_Failure_CBRB_Disabled;
            }
            break;
    }
	return STATE_Failure_CBRB_Disabled;
}

uint32_t cockpitBRBReleased(uint32_t event)
{
	return STATE_HV_Disable;
}
