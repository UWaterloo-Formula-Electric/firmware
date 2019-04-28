#include "controlStateMachine.h"
#include "stm32f7xx_hal.h"
#include "stdbool.h"
#include "freertos.h"
#include "task.h"
#include "cmsis_os.h"
#include "debug.h"
#include "state_machine.h"
#include "BMU_dtc.h"
#include "BMU_can.h"
#include "prechargeDischarge.h"
#include "bsp.h"
#include "watchdog.h"

extern osThreadId PCDCHandle;

FSM_Handle_Struct fsmHandle;

uint32_t runSelftTests(uint32_t event);
uint32_t startPrecharge(uint32_t event);
uint32_t startDischarge(uint32_t event);
uint32_t dischargeFinished(uint32_t event);
uint32_t prechargeFinished(uint32_t event);
uint32_t controlDoNothing(uint32_t event);
uint32_t DefaultTransition(uint32_t event);
uint32_t handleFault(uint32_t event);

Transition_t transitions[] = {
    { STATE_Self_Check, EV_Init, &runSelftTests },
    { STATE_HV_Disable, EV_HV_Toggle, &startPrecharge },
    { STATE_Precharge, EV_Precharge_Finished, &prechargeFinished },
    { STATE_HV_Enable, EV_HV_Toggle, &startDischarge },
    { STATE_Discharge, EV_Discharge_Finished, &dischargeFinished },
    { STATE_Precharge, EV_HV_Fault, &handleFault },
    { STATE_Discharge, EV_HV_Fault, &handleFault },
    { STATE_Precharge, EV_PrechargeDischarge_Fail, &handleFault },
    { STATE_Discharge, EV_PrechargeDischarge_Fail, &handleFault },
    { STATE_HV_Enable, EV_HV_Fault, &handleFault },
    { STATE_HV_Disable, EV_HV_Fault, &handleFault },
    { STATE_Failure_Fatal, EV_ANY, &controlDoNothing },
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
        ERROR_PRINT("Failed to init control fsm\n");
        return HAL_ERROR;
    }

    if (registerTaskToWatch(1, 5, true, &fsmHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    DEBUG_PRINT("Init control fsm\n");
    return HAL_OK;
}

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

uint32_t runSelftTests(uint32_t event)
{
    // TODO: Run some tests
    return STATE_HV_Disable;
}

uint32_t controlDoNothing(uint32_t event)
{
    return fsmGetState(&fsmHandle);
}

uint32_t DefaultTransition(uint32_t event)
{
    ERROR_PRINT("No transition function registered for state %lu, event %lu\n",
                fsmGetState(&fsmHandle), event);

    sendDTC_FATAL_BMU_ERROR();
    return handleFault(EV_HV_Fault);
}

uint32_t startPrecharge(uint32_t event)
{
    DEBUG_PRINT("starting precharge\n");

    xTaskNotify(PCDCHandle, (1<<PRECHARGE_NOTIFICATION), eSetBits);
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
    DEBUG_PRINT("discharge finished\n");

    HV_Power_State = HV_Power_State_Off;
    sendCAN_BMU_HV_Power_State();

    return STATE_HV_Disable;
}

uint32_t prechargeFinished(uint32_t event)
{
    DEBUG_PRINT("precharge finished\n");

    HV_Power_State = HV_Power_State_On;
    sendCAN_BMU_HV_Power_State();

    return STATE_HV_Enable;
}

uint32_t stopDischarge()
{
    DEBUG_PRINT("stopDischarge\n");
    xTaskNotify(PCDCHandle, (1<<STOP_NOTIFICATION), eSetBits);
    return STATE_HV_Enable;
}

void stopPrecharge()
{
    DEBUG_PRINT("stopPrecharge\n");
    xTaskNotify(PCDCHandle, (1<<STOP_NOTIFICATION), eSetBits);
}

uint32_t handleFault(uint32_t event)
{
    sendDTC_FATAL_BMU_ERROR();
    ERROR_PRINT("State machine received fault\n");

    uint32_t currentState = fsmGetState(&fsmHandle);

    switch (currentState) {
        case STATE_HV_Disable:
            {
                DEBUG_PRINT("HV disabled fault, do nothing\n");
            }
            break;
        case STATE_HV_Enable:
            {
                DEBUG_PRINT("hvEnabledHVFault, starting discharge\n");
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
                    stopPrecharge();
                }
            }
            break;
        case STATE_Discharge:
            {
                DEBUG_PRINT("Fault during discharge. Attempting to continue discharge\n");
            }
            break;
        default:
            {
                ERROR_PRINT("HV Fault during other state. Starting discharge\n");
                xTaskNotify(PCDCHandle, (1<<DISCHARGE_NOTIFICATION), eSetBits);
            }
            break;
    }

    return STATE_Failure_Fatal;
}
