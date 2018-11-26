#include "controlStateMachine.h"
#include "stm32f7xx_hal.h"
#include "controlStateMachine_mock.h"
#include "stdbool.h"
#include "freertos.h"
#include "task.h"
#include "debug.h"
#include "state_machine.h"
#include "timers.h"
#include "PDU_dtc.h"
#include "PDU_can.h"

#define HV_CRITICAL_MAIN_DELAY_TIME_MS 1000
#define COOLING_DELAY_TIME_MS 5000

FSM_Handle_Struct mainFsmHandle;
FSM_Handle_Struct motorFsmHandle;
FSM_Handle_Struct coolingFsmHandle;
TimerHandle_t criticalDelayTimer;
TimerHandle_t coolingDelayTimer;

HAL_StatusTypeDef turnBoardsOn();
HAL_StatusTypeDef turnBoardsOff();
uint32_t runSelftTests(uint32_t event);
uint32_t motorsOff(uint32_t event);
uint32_t motorsOn(uint32_t event);
uint32_t motorsOffCritical(uint32_t event);
uint32_t lvCuttoff(uint32_t event);
uint32_t criticalFailure(uint32_t event);
uint32_t criticalFailureWarning(uint32_t event);
uint32_t MainDefaultTransition(uint32_t event);
uint32_t MotorDefaultTransition(uint32_t event);
uint32_t mainDoNothing(uint32_t event);
uint32_t motorDoNothing(uint32_t event);
uint32_t hvEnable(uint32_t event);
uint32_t coolingOn(uint32_t event);
uint32_t coolingOff(uint32_t event);
uint32_t coolingCriticalFailure(uint32_t event);
uint32_t coolingLVCuttoff(uint32_t event);
uint32_t coolingDoNothing(uint32_t event);
uint32_t CoolDefaultTransition(uint32_t event);
void coolingDelayCallback(TimerHandle_t timer);
void hvCriticalDelayCallback(TimerHandle_t timer);
HAL_StatusTypeDef startControl();

Transition_t mainTransitions[] = {
    { MN_STATE_Boards_Off, MN_EV_Init, &runSelftTests },
    { MN_STATE_Boards_On,  MN_EV_HV_CriticalFailure, &criticalFailureWarning },
    { MN_STATE_Warning_Critical, MN_EV_CriticalDelayElapsed, &criticalFailure },
    { MN_STATE_Boards_On, MN_EV_LV_Cuttoff, &lvCuttoff },
    { MN_STATE_Critical_Failure, MN_EV_ANY, &mainDoNothing },
    { MN_STATE_ANY, MN_EV_ANY, &MainDefaultTransition}
};

Transition_t motorTransitions[] = {
    { MTR_STATE_Motors_Off, MTR_EV_EM_ENABLE, &motorsOn },
    { MTR_STATE_Motors_On, MTR_EV_EM_DISABLE, &motorsOff },
    { MTR_STATE_Motors_On, MTR_EV_Motor_Critical, &motorsOff },
    { MTR_STATE_Motors_Off, MTR_EV_Motor_Critical, &motorsOffCritical },
    { MTR_STATE_Critical, MTR_EV_ANY, &motorDoNothing },
    { MTR_STATE_Motors_Off, MTR_EV_EM_DISABLE, &motorDoNothing },
    { MTR_STATE_ANY, MTR_EV_ANY, &MotorDefaultTransition}
};

Transition_t coolingTransitions[] = {
    { COOL_STATE_OFF, COOL_EV_HV_ENABLE, &hvEnable},
    { COOL_STATE_OFF, COOL_EV_OVERTEMP_WARNING, &coolingOn},
    { COOL_STATE_WAIT, COOL_EV_OVERTEMP_WARNING, &coolingOn},
    { COOL_STATE_WAIT, COOL_EV_WAIT_ELAPSED, &coolingOn},
    { COOL_STATE_ON, COOL_EV_HV_DISABLE, &coolingOff},
    { COOL_STATE_ON, COOL_EV_Critical, &coolingCriticalFailure },
    { COOL_STATE_OFF, COOL_EV_Critical, &coolingCriticalFailure },
    { COOL_STATE_WAIT, COOL_EV_Critical, &coolingCriticalFailure },
    { COOL_STATE_ANY, COOL_EV_LV_Cuttoff, &coolingLVCuttoff },
    { COOL_STATE_LV_Cuttoff, COOL_EV_ANY, &coolingDoNothing },
    { COOL_STATE_HV_CRITICAL, COOL_EV_ANY, &coolingDoNothing },
    { COOL_STATE_ON, COOL_EV_OVERTEMP_WARNING, &coolingDoNothing },
    { COOL_STATE_ANY, COOL_EV_ANY, &CoolDefaultTransition}
};


HAL_StatusTypeDef motorControlInit()
{
    FSM_Init_Struct init;

    init.maxStateNum = MTR_STATE_ANY;
    init.maxEventNum = MTR_EV_ANY;
    init.sizeofEventEnumType = sizeof(MotorControl_PDU_Events_t);
    init.ST_ANY = MTR_STATE_ANY;
    init.EV_ANY = MTR_EV_ANY;
    init.transitions = motorTransitions;
    init.transitionTableLength = TRANS_COUNT(motorTransitions);
    init.eventQueueLength = 5;
    if (fsmInit(MTR_STATE_Motors_Off, &init, &motorFsmHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    DEBUG_PRINT("Init motor control\n");
    return HAL_OK;
}

HAL_StatusTypeDef coolingControlInit()
{
    FSM_Init_Struct init;

    coolingDelayTimer = xTimerCreate("COOLING_DELAY",
                                       pdMS_TO_TICKS(COOLING_DELAY_TIME_MS),
                                       pdFALSE /* Auto Reload */,
                                       0,
                                       coolingDelayCallback);

    if (coolingDelayTimer == NULL) {
        ERROR_PRINT("Failed to create software timer\n");
        return HAL_ERROR;
    }

    init.maxStateNum = COOL_STATE_ANY;
    init.maxEventNum = COOL_EV_ANY;
    init.sizeofEventEnumType = sizeof(CoolingControl_PDU_Events_t);
    init.ST_ANY = COOL_STATE_ANY;
    init.EV_ANY = COOL_EV_ANY;
    init.transitions = coolingTransitions;
    init.transitionTableLength = TRANS_COUNT(coolingTransitions);
    init.eventQueueLength = 5;
    if (fsmInit(COOL_STATE_OFF, &init, &coolingFsmHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    DEBUG_PRINT("Init cooling control\n");
    return HAL_OK;
}

HAL_StatusTypeDef maincontrolInit()
{
    FSM_Init_Struct init;

    criticalDelayTimer = xTimerCreate("HV_CRITICAL_DELAY",
                                       pdMS_TO_TICKS(HV_CRITICAL_MAIN_DELAY_TIME_MS),
                                       pdFALSE /* Auto Reload */,
                                       0,
                                       hvCriticalDelayCallback);

    if (criticalDelayTimer == NULL) {
        ERROR_PRINT("Failed to create software timer\n");
        return HAL_ERROR;
    }

    init.maxStateNum = MN_STATE_ANY;
    init.maxEventNum = MN_EV_ANY;
    init.sizeofEventEnumType = sizeof(MAIN_PDU_Events_t);
    init.ST_ANY = MN_STATE_ANY;
    init.EV_ANY = MN_EV_ANY;
    init.transitions = mainTransitions;
    init.transitionTableLength = TRANS_COUNT(mainTransitions);
    init.eventQueueLength = 5;
    if (fsmInit(MN_STATE_Boards_Off, &init, &mainFsmHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    DEBUG_PRINT("Init main control\n");
    return HAL_OK;
}

HAL_StatusTypeDef initStateMachines()
{
    if (maincontrolInit() != HAL_OK) {
        ERROR_PRINT("Failed to init main control fsm\n");
        return HAL_ERROR;
    }

    if (motorControlInit() != HAL_OK) {
        ERROR_PRINT("Failed to init motor control fsm\n");
        return HAL_ERROR;
    }

    if (coolingControlInit() != HAL_OK) {
        ERROR_PRINT("Failed to init cooling control fsm\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

void coolingControlTask(void *pvParameters)
{
    if (canStart(&CAN_HANDLE) != HAL_OK)
    {
        ERROR_PRINT("Failed to start CAN!\n");
        Error_Handler();
    }

    fsmTaskFunction(&coolingFsmHandle);

    for(;;); // Shouldn't reach here
}

void motorControlTask(void *pvParameters)
{
    if (canStart(&CAN_HANDLE) != HAL_OK)
    {
        ERROR_PRINT("Failed to start CAN!\n");
        Error_Handler();
    }

    fsmTaskFunction(&motorFsmHandle);

    for(;;); // Shouldn't reach here
}

void mainControlTask(void *pvParameters)
{
    // Pre send MN_EV_INIT to kick off self tests
    startControl();

    if (canStart(&CAN_HANDLE) != HAL_OK)
    {
        ERROR_PRINT("Failed to start CAN!\n");
        Error_Handler();
    }

    fsmTaskFunction(&mainFsmHandle);

    for(;;); // Shouldn't reach here
}

HAL_StatusTypeDef startControl()
{
    return fsmSendEvent(&mainFsmHandle, MN_EV_Init, portMAX_DELAY /* timeout */); // Force run of self checks
}

uint32_t startCriticalFailureDelay()
{
    sendDTC_FATAL_BOARDS_TURNING_OFF();

    if (xTimerStart(criticalDelayTimer, 100) != pdPASS) {
        ERROR_PRINT("Failed to start critical delay timer\n");
        criticalFailure(MN_EV_CriticalDelayElapsed);
        return MN_STATE_Critical_Failure;
    }

    return MN_STATE_Warning_Critical;
}

uint32_t criticalFailureWarning(uint32_t event)
{
    DEBUG_PRINT("About to turn boards off!\n");
    sendDTC_FATAL_PDU_HV_Critical();

    return startCriticalFailureDelay();
}

uint32_t criticalFailure(uint32_t event)
{
    DEBUG_PRINT("Critical Failure: Turning boards off\n");
    sendDTC_FATAL_BOARDS_OFF();
    turnBoardsOff();
    fsmSendEventUrgent(&motorFsmHandle, MTR_EV_Motor_Critical, 10 /* timeout */);
    fsmSendEventUrgent(&coolingFsmHandle, COOL_EV_Critical, 10 /* timeout */);
    return MN_STATE_Critical_Failure;
}

uint32_t lvCuttoff(uint32_t event)
{
    DEBUG_PRINT("LV Cuttoff, going to critical failure\n");
    sendDTC_FATAL_LV_CUTTOFF_BOARDS_OFF();

    // Cooling needs to handle LV Cuttoff differently than HV Critical, as for
    // critical it can keep cooling on, but for lv cuttoff it needs to turn off
    fsmSendEventUrgent(&coolingFsmHandle, COOL_EV_LV_Cuttoff, 10 /* timeout */);
    return startCriticalFailureDelay();
}

uint32_t runSelftTests(uint32_t event)
{
    // TODO: Run some tests
    DEBUG_PRINT("Running self tests\n");

    turnBoardsOn();
    return MN_STATE_Boards_On;
}

uint32_t MainDefaultTransition(uint32_t event)
{
    ERROR_PRINT("Main FSM: No transition function registered for state %lu, event %lu\n",
                fsmGetState(&mainFsmHandle), event);
    return startCriticalFailureDelay();
}

uint32_t MotorDefaultTransition(uint32_t event)
{
    ERROR_PRINT("Motor FSM: No transition function registered for state %lu, event %lu\n",
                fsmGetState(&motorFsmHandle), event);
    return motorsOff(event);
}

HAL_StatusTypeDef turnBoardsOn()
{
    DEBUG_PRINT("Turning boards on\n");
    return HAL_OK;
}

HAL_StatusTypeDef turnBoardsOff()
{
    DEBUG_PRINT("Turning boards off\n");
    return HAL_OK;
}

uint32_t motorsOn(uint32_t event)
{
    DEBUG_PRINT("Turning motors on\n");
    StatusPowerMCLeft = StatusPowerMCLeft_CHANNEL_ON;
    StatusPowerMCRight = StatusPowerMCRight_CHANNEL_ON;

    if (sendCAN_PDU_ChannelStatus() != HAL_OK) {
        ERROR_PRINT("Failed to send pdu channel status CAN message\n");
        return motorsOff(MTR_EV_EM_DISABLE);
    }
    return MTR_STATE_Motors_On;
}

uint32_t motorsOff(uint32_t event)
{
    DEBUG_PRINT("Turning motors off\n");

    StatusPowerMCLeft = StatusPowerMCLeft_CHANNEL_OFF;
    StatusPowerMCRight = StatusPowerMCRight_CHANNEL_OFF;

    if (sendCAN_PDU_ChannelStatus() != HAL_OK) {
        ERROR_PRINT("Failed to send pdu channel status CAN message\n");
    }

    if (event == MTR_EV_EM_DISABLE) {
        return MTR_STATE_Motors_Off;
    } else {
        return MTR_STATE_Critical;
    }
}

uint32_t motorsOffCritical(uint32_t event)
{
    // Probably don't need to do anything here
    DEBUG_PRINT("Critical failure, motors already off\n");
    return MTR_STATE_Critical;
}

uint32_t motorDoNothing(uint32_t event)
{
    return fsmGetState(&motorFsmHandle);
}
uint32_t mainDoNothing(uint32_t event)
{
    return fsmGetState(&mainFsmHandle);
}

void hvCriticalDelayCallback(TimerHandle_t timer)
{
    if (fsmSendEventUrgent(&mainFsmHandle, MN_EV_CriticalDelayElapsed, 10 /* timeout */) != HAL_OK) {
        ERROR_PRINT("Failed to process critical delay elapsed event\n");
        criticalFailure(MN_EV_CriticalDelayElapsed);
    }
}
void coolingDelayCallback(TimerHandle_t timer)
{
    if (fsmSendEvent(&coolingFsmHandle, COOL_EV_WAIT_ELAPSED, 10 /* timeout */) != HAL_OK) {
        ERROR_PRINT("Failed to process cooling delay elapsed event\n");
        coolingCriticalFailure(COOL_EV_WAIT_ELAPSED);
    }
}

uint32_t CoolDefaultTransition(uint32_t event) {
    ERROR_PRINT("Cooling FSM: No transition function registered for state %lu, event %lu\n",
                fsmGetState(&coolingFsmHandle), event);
    return coolingCriticalFailure(event);
}

uint32_t coolingDoNothing(uint32_t event) {
    DEBUG_PRINT("Cooling do nothing\n");
    return fsmGetState(&coolingFsmHandle);
}

uint32_t coolingLVCuttoff(uint32_t event) {
    DEBUG_PRINT("LV Cuttoff: Turning cooling off\n");
    return COOL_STATE_LV_Cuttoff;
}
uint32_t coolingCriticalFailure(uint32_t event) {
    if (fsmGetState(&coolingFsmHandle) != COOL_STATE_HV_CRITICAL) {
        DEBUG_PRINT("Cooling critical failure\n");
        // TODO: What to do here? Should turn off, or leave on cooling
    } else {
        DEBUG_PRINT("Cooling Already critical\n");
    }

    return COOL_STATE_HV_CRITICAL;
}

uint32_t coolingOff(uint32_t event) {
    DEBUG_PRINT("Turning cooling off\n");
    return COOL_STATE_OFF;
}

uint32_t coolingOn(uint32_t event) {
    DEBUG_PRINT("Turning cooling on\n");
    return COOL_STATE_ON;
}

uint32_t hvEnable(uint32_t event) {
    DEBUG_PRINT("HV Enable received\n");
    if (xTimerStart(coolingDelayTimer, 100) != pdPASS) {
        ERROR_PRINT("Failed to start coolingdelay timer\n");
        coolingOn(COOL_EV_WAIT_ELAPSED);
        return COOL_STATE_ON;
    }

    return COOL_STATE_WAIT;
}
