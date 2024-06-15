
#include "controlStateMachine.h"

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stdbool.h"

#include "bsp.h"
#include "debug.h"
#include "dcu_can.h"
#include "dcu_dtc.h"
#include "userCan.h"
#include "watchdog.h"
#include "canReceive.h"

#define MAIN_TASK_ID 1
#define MAIN_TASK_PERIOD_MS 1000
#define BUZZER_LENGTH_MS 2000
#define DEBOUNCE_WAIT_MS 50
#define EM_BUTTON_RATE_LIMIT_MS 1000  // prevents multiple button presses within this duration

FSM_Handle_Struct DCUFsmHandle;

static uint32_t sendHvToggle(uint32_t event);
static uint32_t sendEmToggle(uint32_t event);
static uint32_t processHvState(uint32_t event);
static uint32_t processEmState(uint32_t event);
static uint32_t fatalTransition(uint32_t event);
static uint32_t toggleTC(uint32_t event);
static uint32_t toggleEnduranceMode(uint32_t event);
static uint32_t sendNewLap(uint32_t event);
static void debounceTimerCallback(TimerHandle_t timer);
static void buzzerTimerCallback(TimerHandle_t timer);
static uint32_t defaultTransition(uint32_t event);
static int sendHVToggleMsg(void);
static int sendEMToggleMsg(void);
static int sendEnduranceToggleMsg(void);
static int sendEnduranceLapMsg(void);
static int sendTCToggleMsg(void);
static int sendScrNavRightEnabled(void);
static int sendScrNavLeftEnabled(void);

static TimerHandle_t buzzerSoundTimer;
static TimerHandle_t debounceTimer;
static bool TC_on = false;
static bool endurance_on = false;
static bool sentFatalDTC = false;
static bool buzzerTimerStarted = false;
static bool alreadyDebouncing = false;
static uint16_t debouncingPin = 0;
static bool isPendingHvResponse = false;
static bool isPendingEmResponse = false;

// This state machine was simplified because the DCU was experiencing stack overflows
Transition_t transitions[] = {
    {STATE_Failure_Fatal,       EV_ANY,                         &fatalTransition},
    {STATE_ANY,                 EV_Fatal,                       &fatalTransition},
    {STATE_ANY,                 EV_CAN_Receive_HV,              &processHvState},
    {STATE_ANY,                 EV_CAN_Receive_EM,              &processEmState},
    {STATE_ANY,                 EV_BTN_HV_Toggle,               &sendHvToggle},
    {STATE_ANY,                 EV_BTN_EM_Toggle,               &sendEmToggle},
    {STATE_EM_Enable,           EV_BTN_TC_Toggle,               &toggleTC},
    {STATE_EM_Enable,           EV_BTN_Endurance_Mode_Toggle,   &toggleEnduranceMode},
    {STATE_EM_Enable,           EV_BTN_Endurance_Lap,           &sendNewLap},
    {STATE_ANY,           EV_ANY,           &defaultTransition},
};

static uint32_t sendHvToggle(uint32_t event)
{
    const DCU_States_t current_state = fsmGetState(&DCUFsmHandle);
    DCU_States_t new_state = STATE_HV_Disable;

    switch (current_state)
    {
        case STATE_Failure_Fatal:
        {
            DEBUG_PRINT("Fault: Shouldn't toggle HV while faulted\r\n");
            sentFatalDTC = true;
            sendDTC_WARNING_DCU_SM_ERROR(2);
            new_state = STATE_Failure_Fatal;
            break;
        }
        case STATE_HV_Disable:
        case STATE_HV_Enable:
        case STATE_EM_Enable:
        {
            DEBUG_PRINT("Sending HV Toggle button event\n");
            if (sendHVToggleMsg() != HAL_OK)
            {
                ERROR_PRINT("Failed to send HV Toggle button event!\n");
                Error_Handler();
            }
            isPendingHvResponse = true;

            new_state = current_state;
            break;
        }
        default:
        {
            DEBUG_PRINT("Fault: Unhandled State for sending HV Toggle\r\n");
            sentFatalDTC = true;
            sendDTC_WARNING_DCU_SM_ERROR(6);
            new_state = STATE_Failure_Fatal;
            break;
        }
    }

    return new_state;
}

bool pendingHvResponse(void)
{
    return isPendingHvResponse == true;
}

void receivedHvResponse(void)
{
    isPendingHvResponse = false;
}

static uint32_t sendEmToggle(uint32_t event)
{
    static TickType_t lastToggleTime = 0U;

    /*
    temporary check to prevent button from double clicking
    remove after 2024 michigan (the issue is probably the button itself??)
    */
    if (xTaskGetTickCount() - lastToggleTime < pdMS_TO_TICKS(EM_BUTTON_RATE_LIMIT_MS))
    {
        DEBUG_PRINT("Ignoring EM Toggle button event\n");
        return fsmGetState(&DCUFsmHandle);
    } else {
        lastToggleTime = xTaskGetTickCount();
    }

    const DCU_States_t current_state = fsmGetState(&DCUFsmHandle);

    if (current_state == STATE_HV_Disable || current_state == STATE_Failure_Fatal)
    {
        sentFatalDTC = true;
        DEBUG_PRINT("Cant EM while in state %d\r\n", current_state);

        sendDTC_WARNING_DCU_SM_ERROR(3);
        return current_state;
    }
    
    DEBUG_PRINT("Sending EM Toggle button event\n");
    if (sendEMToggleMsg() != HAL_OK)
    {
        ERROR_PRINT("Failed to send EM Toggle button event!\n");
        Error_Handler();
    }
    isPendingEmResponse = true;

    return current_state;
}

bool pendingEmResponse(void)
{
    return isPendingEmResponse == true;
}

void receivedEmResponse(void)
{
    isPendingEmResponse = false;
}

static uint32_t processHvState(uint32_t event)
{
    const DCU_States_t current_state = fsmGetState(&DCUFsmHandle);
    if(getHVState() == HV_Power_State_On)
    {
        if (current_state == STATE_HV_Enable || current_state == STATE_EM_Enable)
        {
            // Boards are probably out of sync. Power cycle should fix
            DEBUG_PRINT("FSM already at HV. FAULT\r\n");
            sentFatalDTC = true;
            sendDTC_WARNING_DCU_SM_ERROR(1);
            return STATE_Failure_Fatal;
        }
        DEBUG_PRINT("State: HV Enabled\r\n");
        return STATE_HV_Enable;
    }
    else
    {
        TC_LED_OFF;
        ENDURANCE_LED_OFF;
        if (current_state == STATE_HV_Disable)
        {
            DEBUG_PRINT("Warning: State already HV Disabled\r\n");
        }
        DEBUG_PRINT("State: HV Disabled\r\n");
        return STATE_HV_Disable;
    }
}

static uint32_t processEmState(uint32_t event)
{
    const DCU_States_t current_state = fsmGetState(&DCUFsmHandle);
    DCU_States_t new_state = current_state;

    switch (current_state)
    {
        case STATE_Failure_Fatal:
        {
            DEBUG_PRINT("Can't EM Enable in Fatal State\r\n");
            sentFatalDTC = true;
            sendDTC_WARNING_DCU_SM_ERROR(7);
            new_state = STATE_Failure_Fatal;
            break;
        }
        case STATE_HV_Disable:
        {
            if(getEMState() == EM_State_On)
            {
                DEBUG_PRINT("Can't EM Enable from HV Disable\r\n");
                sentFatalDTC = true;
                sendDTC_WARNING_DCU_SM_ERROR(4);
                new_state = STATE_Failure_Fatal;
            }
            else
            {
                DEBUG_PRINT("VCU fell from EM\r\n");
            }
            break;
        }
        case STATE_HV_Enable:
        {
            if(getEMState() == EM_State_On)
            {
                DEBUG_PRINT("Received EM Enabled\r\n");

                /* Only ring buzzer when going to motors enabled */
                DEBUG_PRINT("Kicking off buzzer\n");
                if (!buzzerTimerStarted)
                {
                    if (xTimerStart(buzzerSoundTimer, 100) != pdPASS)
                    {
                        ERROR_PRINT("Failed to start buzzer timer\n");
                        Error_Handler();
                    }

                    buzzerTimerStarted = true;
                    BUZZER_ON
                }
                new_state = STATE_EM_Enable;
            }
            else
            {
                DEBUG_PRINT("Warning: Received EM Disable while already at State HV\r\n");
            }
            break;
        }
        case STATE_EM_Enable:
        {
            if(getEMState() == EM_State_On)
            {
                DEBUG_PRINT("Warning: Received EM Disable while already at State HV\r\n");
            }
            else
            {
                DEBUG_PRINT("Dropping from EM\r\n");
                TC_LED_OFF;
                ENDURANCE_LED_OFF;
                new_state = STATE_HV_Enable;
            }
            break;
        }
        default:
        {
            DEBUG_PRINT("Unhandled State in processEmState\r\n");
            sentFatalDTC = true;
            sendDTC_WARNING_DCU_SM_ERROR(5);
            break;
        }
    }
    return new_state;
}

static uint32_t fatalTransition(uint32_t event)
{
    DEBUG_PRINT("Fatal Event. Entering Fatal State\r\n");
    if (!sentFatalDTC)
    {
        sentFatalDTC = true;
        DEBUG_PRINT("Sending SM Fatal DTC\r\n");
        sendDTC_WARNING_DCU_SM_ERROR(0);
    }
    return STATE_Failure_Fatal;
}

static uint32_t toggleTC(uint32_t event)
{
    if(sendTCToggleMsg() != HAL_OK)
    {
        ERROR_PRINT("Failed to send TC Toggle button event!\n");
        Error_Handler();
    }

    TC_on = !TC_on;
    if (TC_on)
    {
        TC_LED_ON;
        DEBUG_PRINT("TC on\n");
    }
    else 
    {
        TC_LED_OFF;
        DEBUG_PRINT("TC off\n");
    }
    return STATE_EM_Enable;
}

static uint32_t toggleEnduranceMode(uint32_t event)
{
    if(sendEnduranceToggleMsg() != HAL_OK)
    {
        ERROR_PRINT("Failed to send EnduranceMode Toggle button event!\n");
        Error_Handler();
    }
    endurance_on = !endurance_on;
    if (endurance_on)
    {
        ENDURANCE_LED_ON;
        DEBUG_PRINT("Endurance on\n");
    }
    else 
    {
        ENDURANCE_LED_OFF;
        DEBUG_PRINT("Endurance off\n");
    }
    return STATE_EM_Enable;
}

static uint32_t sendNewLap(uint32_t event)
{
    DEBUG_PRINT("New Lap\r\n");
    if(sendEnduranceLapMsg() != HAL_OK)
    {
        ERROR_PRINT("Failed to send EnduranceLap Toggle button event!\n");
        Error_Handler();
    }
    return STATE_EM_Enable;
}

/*
 * A button press is considered valid if it is still low (active) after
 * TIMER_WAIT_MS milliseconds.
 */
static void debounceTimerCallback(TimerHandle_t timer)
{
    GPIO_PinState pin_val;

    switch (debouncingPin)
    {
        case HV_TOGGLE_BUTTON_PIN:
            pin_val = HAL_GPIO_ReadPin(HV_TOGGLE_BUTTON_PORT,
                    HV_TOGGLE_BUTTON_PIN);
            break;

        case EM_TOGGLE_BUTTON_PIN:
            pin_val = HAL_GPIO_ReadPin(EM_TOGGLE_BUTTON_PORT,
                    EM_TOGGLE_BUTTON_PIN);
            break;
        
        case TC_TOGGLE_BUTTON_PIN:
            pin_val = HAL_GPIO_ReadPin(TC_TOGGLE_BUTTON_PORT,
                    TC_TOGGLE_BUTTON_PIN);
            break;
         
        case ENDURANCE_TOGGLE_BUTTON_PIN:
            pin_val = HAL_GPIO_ReadPin(ENDURANCE_TOGGLE_BUTTON_PORT,
                    ENDURANCE_TOGGLE_BUTTON_PIN);
            break;

        case ENDURANCE_LAP_BUTTON_PIN:
            pin_val = HAL_GPIO_ReadPin(ENDURANCE_LAP_BUTTON_PORT,
                    ENDURANCE_LAP_BUTTON_PIN);
            break;
        
        case SCR_NAV_R_BUTTON_PIN:
            pin_val = HAL_GPIO_ReadPin(SCR_NAV_R_BUTTON_PORT,
                    SCR_NAV_R_BUTTON_PIN);
            break;
        
        case SCR_NAV_L_BUTTON_PIN:
            pin_val = HAL_GPIO_ReadPin(SCR_NAV_L_BUTTON_PORT,
                    SCR_NAV_L_BUTTON_PIN);
            break;

        default:x
            /* Shouldn't get here */ 
            DEBUG_PRINT_ISR("Unknown pin specified to debounce\n");
            pin_val = GPIO_PIN_SET;
            break;
    }


    if (pin_val == GPIO_PIN_RESET)
    {
        switch (debouncingPin)
        {
            case HV_TOGGLE_BUTTON_PIN:
                fsmSendEventISR(&DCUFsmHandle, EV_BTN_HV_Toggle);
                break;

            case EM_TOGGLE_BUTTON_PIN:
                fsmSendEventISR(&DCUFsmHandle, EV_BTN_EM_Toggle);
                break;
            
            case TC_TOGGLE_BUTTON_PIN:
                fsmSendEventISR(&DCUFsmHandle, EV_BTN_TC_Toggle);
                break;

            case ENDURANCE_TOGGLE_BUTTON_PIN:
                fsmSendEventISR(&DCUFsmHandle, EV_BTN_Endurance_Mode_Toggle);
                break;

            case ENDURANCE_LAP_BUTTON_PIN:
                fsmSendEventISR(&DCUFsmHandle, EV_BTN_Endurance_Lap);
                break;
            
            case SCR_NAV_R_BUTTON_PIN:
                sendScrNavRightEnabled();
                break;
            
            case SCR_NAV_L_BUTTON_PIN:
                sendScrNavLeftEnabled();
                break;

            default:
                /* Shouldn't get here */
                DEBUG_PRINT_ISR("Unknown pin specified to debounce\n");
                break;
        }

    }

    alreadyDebouncing = false;
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (alreadyDebouncing)
    {
        /* Already debouncing, do nothing with this interrupt */
        return;
    }
    alreadyDebouncing = true;

    switch (pin)
    {
        case HV_TOGGLE_BUTTON_PIN:
            debouncingPin = HV_TOGGLE_BUTTON_PIN;
            break;

        case EM_TOGGLE_BUTTON_PIN:
            debouncingPin = EM_TOGGLE_BUTTON_PIN;
            break;
        
        case TC_TOGGLE_BUTTON_PIN:
            debouncingPin = TC_TOGGLE_BUTTON_PIN;
            break;

        case ENDURANCE_TOGGLE_BUTTON_PIN:
            debouncingPin = ENDURANCE_TOGGLE_BUTTON_PIN;
            break;

        case ENDURANCE_LAP_BUTTON_PIN:
            debouncingPin = ENDURANCE_LAP_BUTTON_PIN;
            break;
        
        case SCR_NAV_R_BUTTON_PIN:
            debouncingPin = SCR_NAV_R_BUTTON_PIN;
            break;
        
        case SCR_NAV_L_BUTTON_PIN:
            debouncingPin = SCR_NAV_L_BUTTON_PIN;
            break;
        
        default:
            /* Not a fatal error here, but report error and return */
            DEBUG_PRINT_ISR("Unknown GPIO interrupted in ISR!\n");
            return;
            break;
    }

    xTimerStartFromISR(debounceTimer, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

HAL_StatusTypeDef dcuFsmInit(){
    FSM_Init_Struct init;
    init.maxStateNum = STATE_ANY;
    init.maxEventNum = EV_ANY;
    init.sizeofEventEnumType = sizeof(DCU_Events_t);
    init.ST_ANY = STATE_ANY;
    init.EV_ANY = EV_ANY;
    init.transitions = transitions;
    init.transitionTableLength = TRANS_COUNT(transitions);
    init.eventQueueLength = 5;
    init.watchdogTaskId = MAIN_TASK_ID;
    if (fsmInit(STATE_HV_Disable, &init, &DCUFsmHandle) != HAL_OK) 
    {
        ERROR_PRINT("Failed to init DCU fsm\n");
        return HAL_ERROR;
    }

    DEBUG_PRINT("Init DCU fsm\n");
    return HAL_OK;
}

static void buzzerTimerCallback(TimerHandle_t timer)
{
    buzzerTimerStarted = false;
    BUZZER_OFF
}

void mainTaskFunction(void const * argument) {
    DEBUG_PRINT("Starting up!!\n");
    if (canStart(&CAN_HANDLE) != HAL_OK)
    {
        ERROR_PRINT("Failed to start CAN!\n");
        Error_Handler();
    }

    buzzerSoundTimer = xTimerCreate("BuzzerTimer",
                                    pdMS_TO_TICKS(BUZZER_LENGTH_MS),
                                    pdFALSE /* Auto Reload */,
                                    0,
                                    buzzerTimerCallback);

    if (buzzerSoundTimer == NULL)
    {
        ERROR_PRINT("Failed to create buzzer timer!\n");
        Error_Handler();
    }

    debounceTimer = xTimerCreate("DebounceTimer",
                                 pdMS_TO_TICKS(DEBOUNCE_WAIT_MS),
                                 pdFALSE /* Auto Reload */,
                                 0,
                                 debounceTimerCallback);

    if (debounceTimer == NULL) 
    {
        ERROR_PRINT("Failed to create debounce timer!\n");
        Error_Handler();
    }


    if (registerTaskToWatch(MAIN_TASK_ID, 5*pdMS_TO_TICKS(MAIN_TASK_PERIOD_MS), true, &DCUFsmHandle) != HAL_OK)
    {
        ERROR_PRINT("Failed to register main task with watchdog!\n");
        Error_Handler();
    }

    fsmTaskFunction(&DCUFsmHandle);

    for(;;) {
    };
}

static uint32_t defaultTransition(uint32_t event)
{
    const uint32_t currentState = fsmGetState(&DCUFsmHandle);
    ERROR_PRINT("No transition function for state %lu and event %lu\r\n", currentState, event);
    sendDTC_FATAL_DCU_NoTransition();
    return currentState;
}

static int sendHVToggleMsg(void)
{
    ButtonHVEnabled = 1;
    ButtonEMEnabled = 0;
    ButtonEnduranceToggleEnabled = 0;
    ButtonEnduranceLapEnabled = 0;
    ButtonTCEnabled = 0;
    ButtonScreenNavRightEnabled = 0;
    ButtonScreenNavLeftEnabled = 0;
    return sendCAN_DCU_buttonEvents();
}

static int sendEMToggleMsg(void)
{
    ButtonHVEnabled = 0;
    ButtonEMEnabled = 1;
    ButtonEnduranceToggleEnabled = 0;
    ButtonEnduranceLapEnabled = 0;
    ButtonTCEnabled = 0;
    ButtonScreenNavRightEnabled = 0;
    ButtonScreenNavLeftEnabled = 0;
    return sendCAN_DCU_buttonEvents();
}

static int sendEnduranceToggleMsg(void)
{
    ButtonHVEnabled = 0;
    ButtonEMEnabled = 0;
    ButtonEnduranceToggleEnabled = 1;
    ButtonEnduranceLapEnabled = 0;
    ButtonTCEnabled = 0;
    ButtonScreenNavRightEnabled = 0;
    ButtonScreenNavLeftEnabled = 0;
    return sendCAN_DCU_buttonEvents();
}

static int sendEnduranceLapMsg(void)
{
    ButtonHVEnabled = 0;
    ButtonEMEnabled = 0;
    ButtonEnduranceToggleEnabled = 0;
    ButtonEnduranceLapEnabled = 1;
    ButtonTCEnabled = 0;
    ButtonScreenNavRightEnabled = 0;
    ButtonScreenNavLeftEnabled = 0;
    return sendCAN_DCU_buttonEvents();
}

static int sendTCToggleMsg(void)
{
    ButtonHVEnabled = 0;
    ButtonEMEnabled = 0;
    ButtonEnduranceToggleEnabled = 0;
    ButtonEnduranceLapEnabled = 0;
    ButtonTCEnabled = 1;
    ButtonScreenNavRightEnabled = 0;
    ButtonScreenNavLeftEnabled = 0;
    return sendCAN_DCU_buttonEvents();
}

static int sendScrNavRightEnabled(void)
{
    ButtonHVEnabled = 0;
    ButtonEMEnabled = 0;
    ButtonEnduranceToggleEnabled = 0;
    ButtonEnduranceLapEnabled = 0;
    ButtonTCEnabled = 0;
    ButtonScreenNavRightEnabled = 1;
    ButtonScreenNavLeftEnabled = 0;
    return sendCAN_DCU_buttonEvents();
}

static int sendScrNavLeftEnabled(void)
{
    ButtonHVEnabled = 0;
    ButtonEMEnabled = 0;
    ButtonEnduranceToggleEnabled = 0;
    ButtonEnduranceLapEnabled = 0;
    ButtonTCEnabled = 0;
    ButtonScreenNavRightEnabled = 0;
    ButtonScreenNavLeftEnabled = 1;
    return sendCAN_DCU_buttonEvents();
}