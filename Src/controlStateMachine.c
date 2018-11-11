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

#define HV_CRITICAL_MAIN_DELAY_TIME_MS 1000

FSM_Handle_Struct mainFsmHandle;
TimerHandle_t criticalDelayTimes;

uint32_t runSelftTests(uint32_t event);
HAL_StatusTypeDef turnBoardsOn();
HAL_StatusTypeDef turnBoardsOff();
uint32_t lvCuttoff(uint32_t event);
uint32_t criticalFailure(uint32_t event);
uint32_t criticalFailureWarning(uint32_t event);
uint32_t DefaultTransition(uint32_t event);
void hvCriticalDelayCallback(TimerHandle_t timer);
HAL_StatusTypeDef startControl();

Transition_t mainTransitions[] = {
    { STATE_Boards_Off, EV_Init, &runSelftTests },
    { STATE_Boards_On,  EV_HV_CriticalFailure, &criticalFailureWarning },
    { STATE_Warning_Critical, EV_CriticalDelayElapsed, &criticalFailure },
    { STATE_Boards_On, EV_LV_Cuttoff, &lvCuttoff },
    { STATE_ANY, EV_ANY, &DefaultTransition}
};

HAL_StatusTypeDef maincontrolInit()
{
    FSM_Init_Struct init;

    criticalDelayTimes = xTimerCreate("HV_CRITICAL_DELAY",
                                       pdMS_TO_TICKS(HV_CRITICAL_MAIN_DELAY_TIME_MS),
                                       pdFALSE /* Auto Reload */,
                                       0,
                                       hvCriticalDelayCallback);

    if (criticalDelayTimes == NULL) {
        ERROR_PRINT("Failed to create software timer\n");
        return HAL_ERROR;
    }

    init.maxStateNum = STATE_ANY;
    init.maxEventNum = EV_ANY;
    init.sizeofEventEnumType = sizeof(MAIN_PDU_Events_t);
    init.ST_ANY = STATE_ANY;
    init.EV_ANY = EV_ANY;
    init.transitions = mainTransitions;
    init.transitionTableLength = TRANS_COUNT(mainTransitions);
    init.eventQueueLength = 5;
    fsmInit(STATE_Boards_Off, &init, &mainFsmHandle);

    DEBUG_PRINT("Init main control\n");
    return HAL_OK;
}

HAL_StatusTypeDef initStateMachines()
{
    if (maincontrolInit() != HAL_OK) {
        ERROR_PRINT("Failed to init main control fsm\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

void mainControlTask(void *pvParameters)
{
    // Pre send EV_INIT to kick off self tests
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
    return fsmSendEvent(&mainFsmHandle, EV_Init, portMAX_DELAY /* timeout */); // Force run of self checks
}

uint32_t startCriticalFailureDelay()
{
    sendDTC_FATAL_BOARDS_TURNING_OFF();

    if (xTimerStart(criticalDelayTimes, 100) != pdPASS) {
        ERROR_PRINT("Failed to start critical delay timer\n");
        criticalFailure(EV_CriticalDelayElapsed);
        return STATE_Critical_Failure;
    }

    return STATE_Warning_Critical;
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
    return STATE_Critical_Failure;
}

uint32_t lvCuttoff(uint32_t event)
{
    DEBUG_PRINT("LV Cuttoff, turning boards off\n");
    sendDTC_FATAL_LV_CUTTOFF_BOARDS_OFF();

    return startCriticalFailureDelay();
}

uint32_t runSelftTests(uint32_t event)
{
    // TODO: Run some tests
    DEBUG_PRINT("Running self tests\n");

    turnBoardsOn();
    return STATE_Boards_On;
}

uint32_t DefaultTransition(uint32_t event)
{
    ERROR_PRINT("No transition function registered for state %lu, event %lu\n",
                fsmGetState(&mainFsmHandle), event);
    return startCriticalFailureDelay();
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

void hvCriticalDelayCallback(TimerHandle_t timer)
{
    if (fsmSendEventUrgent(&mainFsmHandle, EV_CriticalDelayElapsed, 0) != HAL_OK) {
        ERROR_PRINT("Failed to process critical delay elapsed event\n");
    }
}
