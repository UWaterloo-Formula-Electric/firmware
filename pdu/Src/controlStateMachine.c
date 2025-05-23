#include "controlStateMachine.h"
#include "stm32f7xx_hal.h"
#include "controlStateMachine_mock.h"
#include "stdbool.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "state_machine.h"
#include "timers.h"
#include "pdu_dtc.h"
#include "pdu_can.h"
#include "loadSensor.h"
#include "bsp.h"
#include "watchdog.h"

#define HV_CRITICAL_MAIN_DELAY_TIME_MS 1000
#define LV_SHUTDOWN_DELAY_TIME_MS 1000
#define COOLING_DELAY_TIME_MS 1000

FSM_Handle_Struct mainFsmHandle;
TimerHandle_t criticalDelayTimer;

uint32_t runSelfTests(uint32_t event);
uint32_t motorsOff(uint32_t event);
uint32_t motorsOn(uint32_t event);
uint32_t criticalFailure(uint32_t event);
uint32_t criticalFailureWarning(uint32_t event);
uint32_t MainDefaultTransition(uint32_t event);
uint32_t mainDoNothing(uint32_t event);
uint32_t cycleMC(uint32_t event);
void hvCriticalDelayCallback(TimerHandle_t timer);
HAL_StatusTypeDef startControl();

Transition_t mainTransitions[] = {
    { STATE_Boards_Off, EV_Init, &runSelfTests },
    { STATE_Boards_On, EV_EM_Enable, &motorsOn }, 
    { STATE_Motors_On, EV_EM_Disable, &motorsOff },
    { STATE_Boards_On, EV_EM_Disable, &mainDoNothing },
    { STATE_Motors_On, EV_EM_Enable, &mainDoNothing },
    { STATE_Boards_On,  EV_HV_CriticalFailure, &criticalFailureWarning },
    { STATE_Motors_On, EV_HV_CriticalFailure, &criticalFailureWarning },
    { STATE_Warning_Critical, EV_CriticalDelayElapsed, &criticalFailure },
    { STATE_Critical_Failure, EV_ANY, &mainDoNothing },
    { STATE_ANY, EV_Cycle_MC, &cycleMC },
    { STATE_Warning_Critical, EV_ANY, &mainDoNothing },
    { STATE_ANY, EV_ANY, &MainDefaultTransition}
};

/* 
 * This is a temporary fix from the 2024 competition where the inverter would occassionally experience HW
 * overcurrent fault. We power cycle the inverter to eliminate the fault. 
 * TODO: once this fault is permanently fixed, this should be removed
 */

// TODO: Rename cycleMC to readMCFault
uint32_t cycleMC(uint32_t event)
{
    extern uint8_t resetting;
    extern uint64_t inverterFaultCode;

    uint32_t invRunFault = (uint32_t)(inverterFaultCode & 0xFFFFFFFF);
    uint32_t invPostFault = (uint32_t)(inverterFaultCode >> 32);

    DEBUG_PRINT("inverter fault. Post: %lu. Run: %lu\r\n", invPostFault, invRunFault);
    
    if (invPostFault) {
        sendDTC_WARNING_PDU_Inverter_Post_Fault(invPostFault);
    } else if (invRunFault) {
        sendDTC_WARNING_PDU_Inverter_Run_Fault(invRunFault);
    }

    // TODO: remove this temporary fix below
    uint32_t current_state = fsmGetState(&mainFsmHandle);
    if (current_state == STATE_Motors_On) {
        INVERTER_DISABLE;
        vTaskDelay(pdMS_TO_TICKS(50));
        INVERTER_EN;
        resetting = 0U;
        return STATE_Motors_On;
    }
    return current_state;
}

HAL_StatusTypeDef mainControlInit()
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
    init.maxStateNum = STATE_ANY;
    init.maxEventNum = EV_ANY;
    init.sizeofEventEnumType = sizeof(MAIN_PDU_Events_t);
    init.ST_ANY = STATE_ANY;
    init.EV_ANY = EV_ANY;
    init.transitions = mainTransitions;
    init.transitionTableLength = TRANS_COUNT(mainTransitions);
    init.eventQueueLength = 5;
    init.watchdogTaskId = MAIN_CONTROL_TASK_ID;
    if (fsmInit(STATE_Boards_Off, &init, &mainFsmHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    if (registerTaskToWatch(MAIN_CONTROL_TASK_ID, 50, true, &mainFsmHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    DEBUG_PRINT("Init main control\n");
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
    if (xTimerStart(criticalDelayTimer, 100) != pdPASS) {
        ERROR_PRINT("Failed to start critical delay timer\n");
        return criticalFailure(EV_CriticalDelayElapsed);
    }

    return STATE_Warning_Critical;
}

uint32_t criticalFailureWarning(uint32_t event)
{
    sendDTC_FATAL_PDU_HV_Critical();

    return startCriticalFailureDelay();
}

uint32_t criticalFailure(uint32_t event)
{
    DEBUG_PRINT("Critical Failure %lu: Boards will remain on\n", event);
    // Cooling unaffected
    // Motor controller needs to be disabled
    return motorsOff(event);
}

uint32_t runSelfTests(uint32_t event)
{
    // TODO: Future task for new members
    DEBUG_PRINT("Running self tests\n");

    turnBoardsOn();
    return STATE_Boards_On;
}

uint32_t MainDefaultTransition(uint32_t event)
{
    ERROR_PRINT("Main FSM: No transition function registered for state %lu, event %lu\n",
                fsmGetState(&mainFsmHandle), event);
    return startCriticalFailureDelay();
}

HAL_StatusTypeDef turnBoardsOn()
{
    DEBUG_PRINT("Turning boards on\n");

    CDU_EN;
    TCU_EN;
    WSB_EN;
    BMU_EN;
    TRANSPONDER_EN;   // TODO: might be for the transponder (needs to be specced)

    StatusPowerCDU = StatusPowerCDU_CHANNEL_ON;
    StatusPowerBMU = StatusPowerBMU_CHANNEL_ON;
    StatusPowerWSB = StatusPowerWSB_CHANNEL_ON;
    StatusPowerTCU = StatusPowerTCU_CHANNEL_ON;

    if (sendCAN_PDU_ChannelStatus() != HAL_OK) {
        ERROR_PRINT("Failed to send pdu channel status CAN message\n");
    }

    return HAL_OK;
}

HAL_StatusTypeDef turnBoardsOff()
{
    DEBUG_PRINT("Turning boards off\n");
    CDU_DISABLE;
    BMU_DISABLE;
    WSB_DISABLE;
    TCU_DISABLE;

    StatusPowerCDU = StatusPowerCDU_CHANNEL_OFF;
    StatusPowerBMU = StatusPowerBMU_CHANNEL_OFF;
    StatusPowerWSB = StatusPowerWSB_CHANNEL_OFF;
    StatusPowerTCU = StatusPowerTCU_CHANNEL_OFF;

    if (sendCAN_PDU_ChannelStatus() != HAL_OK) {
        ERROR_PRINT("Failed to send pdu channel status CAN message\n");
    }
    return HAL_OK;
}

void toggleChannel(uint8_t channel, uint8_t On)
{
    switch (channel)
    {
        case Pump_1_Channel:
            if (On) { PUMP_1_EN; } else { PUMP_1_DISABLE; }
            break;
        case Pump_2_Channel: 
            if (On) { PUMP_2_EN; } else { PUMP_2_DISABLE; }
            break;
        case CDU_Channel:
            if (On) { CDU_EN; } else { CDU_DISABLE; }
            break;
        case BMU_Channel:
            if (On) { BMU_EN; } else { BMU_DISABLE; }
            break;
        case WSB_Channel:
            if (On) { WSB_EN; } else { WSB_DISABLE; }
            break;
        case TCU_Channel:
            if (On) { TCU_EN; } else { TCU_DISABLE; }
            break;
        case Brake_Light_Channel:
            if (On) { BRAKE_LIGHT_ENABLE; } else { BRAKE_LIGHT_DISABLE; }
            break;
        case ACC_Fans_Channel:
            if (On) { ACC_FANS_EN; } else { ACC_FANS_DISABLE; }
            break;
        case INV_Channel:
            if (On) { INVERTER_EN; } else { INVERTER_DISABLE; }
            break;
        case Radiator_Channel:
            if (On) { RADIATOR_EN; } else { RADIATOR_DISABLE; }
            break;
        case AUX_1_Channel:
            if (On) { TRANSPONDER_EN; } else { TRANSPONDER_DISABLE; }
            break;
        case AUX_2_Channel:
            if (On) { AUX_2_EN; } else { AUX_2_DISABLE; }
            break;
        case AUX_3_Channel:
            if (On) { AUX_3_EN; } else { AUX_3_DISABLE; }
            break;
        case AUX_4_Channel:
            if (On) { AUX_4_EN; } else { AUX_4_DISABLE; }
            break;
        default:
            ERROR_PRINT("Error: Reached default case in toggleChannel\r\n");
            break;
    }
}

uint32_t motorsOn(uint32_t event)
{
    DEBUG_PRINT("Turning motors on\n");
    if (fsmGetState(&mainFsmHandle) == STATE_Boards_On) {
        INVERTER_EN;
    }

    StatusPowerInverter = StatusPowerInverter_CHANNEL_ON;

    if (sendCAN_PDU_ChannelStatus() != HAL_OK) {
        ERROR_PRINT("Failed to send pdu channel status CAN message\n");
        return motorsOff(EV_EM_Disable);
    }
    return STATE_Motors_On;
}

uint32_t motorsOff(uint32_t event)
{
    DEBUG_PRINT("Turning motors off\n");

    INVERTER_DISABLE;

    StatusPowerInverter = StatusPowerInverter_CHANNEL_OFF;

    if (sendCAN_PDU_ChannelStatus() != HAL_OK) {
        ERROR_PRINT("Failed to send pdu channel status CAN message\n");
    }

    if (event == EV_EM_Disable) {
        return STATE_Boards_On;
    } else {
        return STATE_Critical_Failure;
    }
}

uint32_t mainDoNothing(uint32_t event)
{
    return fsmGetState(&mainFsmHandle);
}

void hvCriticalDelayCallback(TimerHandle_t timer)
{
    if (fsmSendEventUrgent(&mainFsmHandle, EV_CriticalDelayElapsed, 10 /* timeout */) != HAL_OK) {
        ERROR_PRINT("Failed to process critical delay elapsed event\n");
        criticalFailure(EV_CriticalDelayElapsed);
    }
}
