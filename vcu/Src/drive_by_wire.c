/**
 *******************************************************************************
 * @file    drive_by_wire.c
 * @author	Richard
 * @date    Dec 2024
 * @brief   VCU's state machine, logic to go to EM, and starting the motor controller
 *
 ******************************************************************************
 */

#include "drive_by_wire.h"
#include "stm32f7xx_hal.h"
#include "drive_by_wire_mock.h"
#include "bsp.h"
#include "stdbool.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "debug.h"
#include "state_machine.h"
#include "timers.h"
#include "vcu_F7_dtc.h"
#include "vcu_F7_can.h"
#include "canReceive.h"
#include "brakeAndThrottle.h"
#include "watchdog.h"
#include "motorController.h"
#include "endurance_mode.h"
#include "traction_control.h"

#define DRIVE_BY_WIRE_TASK_ID 1

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/

FSM_Handle_Struct VCUFsmHandle;

uint32_t runSelfTests(uint32_t event);
uint32_t EM_Enable(uint32_t event);
uint32_t EM_Fault(uint32_t event);
uint32_t EM_Update_Throttle(uint32_t event);
static uint32_t DefaultTransition(uint32_t event);

void throttleTimerCallback(TimerHandle_t timer);
HAL_StatusTypeDef MotorStart();
HAL_StatusTypeDef MotorStop();

extern osThreadId throttlePollingHandle;

/* From the DCU */
#define BUZZER_LENGTH_MS 2000
#define DEBOUNCE_WAIT_MS 50
#define EM_BUTTON_RATE_LIMIT_MS 1000  // prevents multiple button presses within this duration


// TODO: can definitely be optimized later
static uint32_t sendHvToggle(uint32_t event);
static uint32_t sendEmToggle(uint32_t event);
static uint32_t processHvState(uint32_t event);
// static uint32_t processEmState(uint32_t event);
static uint32_t fatalTransition(uint32_t event);
static uint32_t toggleTC(uint32_t event);
static uint32_t toggleEnduranceMode(uint32_t event);
// static void debounceTimerCallback(TimerHandle_t timer);
static void buzzerTimerCallback(TimerHandle_t timer);
static int sendHVToggleMsg(void);
static int sendEMToggleMsg(void);
static int sendEnduranceToggleMsg(void);
static int sendTCToggleMsg(void);

static TimerHandle_t buzzerSoundTimer;
// static TimerHandle_t debounceTimer;
static bool TC_on = false;
static bool endurance_on = false;
static bool sentFatalDTC = false;
static bool buzzerTimerStarted = false;
// static bool alreadyDebouncing = false;
// static uint16_t debouncingPin = 0;
static bool isPendingHvResponse = false;

// TODO: clean up this state machine
Transition_t transitions[] = {
    { STATE_Self_Check, EV_Init, &runSelfTests },
    { STATE_HV_Disable, EV_Bps_Fail, &EM_Fault },
    { STATE_HV_Disable, EV_Hv_Disable, &EM_Fault },
    { STATE_HV_Disable, EV_Brake_Pressure_Fault, &EM_Fault },
    { STATE_HV_Disable, EV_Throttle_Failure, &EM_Fault },
    { STATE_HV_Enable, EV_EM_Toggle, &EM_Enable },
    { STATE_HV_Enable, EV_Hv_Disable, & EM_Fault},
    { STATE_EM_Enable, EV_Bps_Fail, &EM_Fault },
    { STATE_EM_Enable, EV_Hv_Disable, &EM_Fault },
    { STATE_EM_Enable, EV_Brake_Pressure_Fault, &EM_Fault },
    { STATE_EM_Enable, EV_Throttle_Failure, &EM_Fault },
    { STATE_EM_Enable, EV_EM_Toggle, &EM_Fault },
    { STATE_Failure_Fatal, EV_ANY, &fatalTransition },
    { STATE_ANY, EV_CAN_Receive_HV, &processHvState},       // From DCU. Check HV toggle response from the BMU
    // { STATE_ANY, EV_CAN_Receive_EM, &processEmState},       // From DCU. Happens locally now so remove it
    { STATE_ANY, EV_BTN_HV_Toggle, &sendHvToggle},          // From DCU
    { STATE_ANY, EV_BTN_EM_Toggle, &sendEmToggle},          // From DCU
    { STATE_EM_Enable, EV_BTN_TC_Toggle, &toggleTC},        // From DCU
    { STATE_EM_Enable, EV_BTN_Endurance_Mode_Toggle, &toggleEnduranceMode},     // From DCU
    { STATE_ANY, EV_Fatal, &EM_Fault },
    { STATE_ANY, EV_ANY, &DefaultTransition}
};

/*********************************************************************************************************************/
/*-----------------------------------------------------Helpers-------------------------------------------------------*/
/*********************************************************************************************************************/
HAL_StatusTypeDef driveByWireInit()
{
    FSM_Init_Struct init;

    init.maxStateNum = STATE_ANY;
    init.maxEventNum = EV_ANY;
    init.sizeofEventEnumType = sizeof(VCU_Events_t);
    init.ST_ANY = STATE_ANY;
    init.EV_ANY = EV_ANY;
    init.transitions = transitions;
    init.transitionTableLength = TRANS_COUNT(transitions);
    init.eventQueueLength = 5;
    init.watchdogTaskId = DRIVE_BY_WIRE_TASK_ID;
    if (fsmInit(STATE_Self_Check, &init, &VCUFsmHandle) != HAL_OK) {
        ERROR_PRINT("Failed to init drive by wire fsm\n");
        return HAL_ERROR;
    }

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

    // debounceTimer = xTimerCreate("DebounceTimer",
    //                              pdMS_TO_TICKS(DEBOUNCE_WAIT_MS),
    //                              pdFALSE /* Auto Reload */,
    //                              0,
    //                              debounceTimerCallback);

    // if (debounceTimer == NULL) 
    // {
    //     ERROR_PRINT("Failed to create debounce timer!\n");
    //     Error_Handler();
    // }

    if (registerTaskToWatch(DRIVE_BY_WIRE_TASK_ID, pdMS_TO_TICKS(DRIVE_BY_WIRE_WATCHDOG_TIMEOUT_MS),
                            true, &VCUFsmHandle) != HAL_OK)
    {
        return HAL_ERROR;
    }

    DEBUG_PRINT("Init drive by wire\n");
    return HAL_OK;
}

// State machine task
void driveByWireTask(void *pvParameters)
{
    // Pre send EV_INIT to kick off self tests
    startDriveByWire();

    if (canStart(&CAN_HANDLE) != HAL_OK) {
        Error_Handler();
    }

    fsmTaskFunction(&VCUFsmHandle);

    for(;;); // Shouldn't reach here
}

HAL_StatusTypeDef startDriveByWire()
{
    return fsmSendEvent(&VCUFsmHandle, EV_Init, portMAX_DELAY /* timeout */); // Force run of self checks
}

uint32_t runSelfTests(uint32_t event)
{
    // TODO: Run some tests
    
    if (brakeAndThrottleStart() != HAL_OK)
    {
        sendDTC_WARNING_Throttle_Failure(3);
        return EM_Fault(EV_Throttle_Failure);
    }

    return STATE_HV_Disable;
}

uint32_t EM_Enable(uint32_t event)
{
    bool hvEnable = getHvEnableState();
    uint32_t state = STATE_EM_Enable;

    // TODO: reintroduce this once brake pressure reading is validated and calibrated!
    // These brake pressure checks were commented out as the sensor was not connected at 2024 Hybrid. They should be reintroduced.
    bool bpsState = checkBPSState();
    float brakePressure = getBrakePressure();
    if (!bpsState) {
        DEBUG_PRINT("Failed to em enable, bps fault\n");
        sendDTC_WARNING_EM_ENABLE_FAILED(0);
        state = STATE_HV_Enable;
    } else if (!(brakePressure > MIN_BRAKE_PRESSURE)) {
        DEBUG_PRINT("Failed to em enable, brake pressure low (%f)\n", brakePressure);
        sendDTC_WARNING_EM_ENABLE_FAILED(1);
        state = STATE_HV_Enable;
    } else if (!(throttleIsZero())) {
        DEBUG_PRINT("Failed to em enable, non-zero throttle\n");
        sendDTC_WARNING_EM_ENABLE_FAILED(2);
        state = STATE_HV_Enable;
    } else if (!isBrakePressed()) {
        DEBUG_PRINT("Failed to em enable, brake is not pressed\n");
        sendDTC_WARNING_EM_ENABLE_FAILED(3);
        state = STATE_HV_Enable;
    } else if (!hvEnable) {
        sendDTC_WARNING_EM_ENABLE_FAILED(4);
        DEBUG_PRINT("Failed to em enable, not HV enabled\n");
        state = STATE_HV_Disable;
    }

    if (state != STATE_EM_Enable) {
        EM_State = (state == STATE_EM_Enable)?EM_State_On:EM_State_Off;
        sendCAN_VCU_EM_State();
        return state;
    }

    DEBUG_PRINT("Trans to em enable\n");
    HAL_StatusTypeDef rc;
    rc = MotorStart();
    if (rc != HAL_OK) {
        ERROR_PRINT("Failed to turn on motors\n");
        if (rc == HAL_TIMEOUT) {
            sendDTC_WARNING_EM_ENABLE_FAILED(5);
            state = STATE_HV_Enable;
        } else {
            sendDTC_FATAL_EM_ENABLE_FAILED(6);
            state = STATE_Failure_Fatal;
        }
    }

	endurance_mode_EM_callback();

    EM_State = (state == STATE_EM_Enable)?EM_State_On:EM_State_Off;
    sendCAN_VCU_EM_State();

    if (state == STATE_EM_Enable)
    {
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
            BUZZER_ENABLE;
        }
    }

    return state;
}

uint32_t EM_Fault(uint32_t event)
{
    int newState = STATE_Failure_Fatal;
    int currentState = fsmGetState(&VCUFsmHandle);
    
    EMFaultEvent = event;
    sendCAN_VCU_EM_Fault();

    if (fsmGetState(&VCUFsmHandle) == STATE_Failure_Fatal) {
        DEBUG_PRINT("EM Fault, already in fatal failure state\n");
        return STATE_Failure_Fatal;
    }


    switch (event) {
        case EV_Bps_Fail:
            {
                sendDTC_CRITICAL_BPS_FAIL();
                DEBUG_PRINT("Bps failed, trans to fatal\n");
                newState = STATE_Failure_Fatal;
            }
            break;
        case EV_Brake_Pressure_Fault:
            {
                sendDTC_CRITICAL_Brake_Pressure_FAIL();
                DEBUG_PRINT("Brake pressure fault, trans to fatal failure\n");
                newState = STATE_Failure_Fatal;
            }
            break;
        case EV_Throttle_Failure:
            {
                DEBUG_PRINT("Throttle read failure, trans to fatal failure\n");
                newState = STATE_Failure_Fatal;
                sendDTC_FATAL_VCU_F7_EV_FATAL();
            }
            break;
        case EV_Hv_Disable:
            {
                if (currentState == STATE_HV_Enable) {
                    DEBUG_PRINT("HV Disable, staying in EM Disabled state\n");
                } else {
                    //disable TC
                    disable_TC();
                    DEBUG_PRINT("HV Disable, trans to EM Disabled\n");

                    // TODO: decide whether the MC should be on/off when the CBRB is pressed.
                    //       might be good for logging if we keep it on 
                    // Turn off MC when CBRB pressed while in EM
                    // if (MotorStop() != HAL_OK) {
                    //    ERROR_PRINT("Failed to stop motors\n");
                    // }

                    EM_State = EM_State_Off;
                    sendCAN_VCU_EM_State();
                }
                newState = STATE_HV_Disable;
            }
            break;
        case EV_EM_Toggle:
            {
                DEBUG_PRINT("EM Toggle, trans to EM Disabled\n");
                //disable TC
                disable_TC();
                newState = STATE_HV_Enable;
            }
            break;
        case EV_Fatal:
            {
                DEBUG_PRINT("Received fatal event, trans to fatal failure\n");
                sendDTC_FATAL_VCU_F7_EV_FATAL();
                newState = STATE_Failure_Fatal;
            }
            break;
        default:
            {
                sendDTC_FATAL_VCU_F7_EM_ENABLED_ERROR();
                DEBUG_PRINT("EM Enabled, unknown event %lu\n", event);
            }
            break;
    }

    if (fsmGetState(&VCUFsmHandle) == STATE_EM_Enable) {
        if (MotorStop() != HAL_OK) {
            ERROR_PRINT("Failed to stop motors\n");
            newState = STATE_Failure_Fatal;
        }

        EM_State = EM_State_Off;
        sendCAN_VCU_EM_State();
    }

    return newState;
}

static uint32_t DefaultTransition(uint32_t event)
{
    ERROR_PRINT("No transition function registered for state %lu, event %lu\n",
                fsmGetState(&VCUFsmHandle), event);

    sendDTC_FATAL_VCU_F7_NoTransition();
    if (MotorStop() != HAL_OK) {
        ERROR_PRINT("Failed to stop motors\n");
    }
    return STATE_Failure_Fatal;
}

HAL_StatusTypeDef turnOnMotorController() {
    uint32_t dbwTaskNotifications;

    // Request PDU to turn on motor controllers
    EM_Power_State_Request = EM_Power_State_Request_On;
    sendCAN_VCU_EM_Power_State_Request();

    // Wait for PDU to turn on MCs
    BaseType_t rc = xTaskNotifyWait( 0x00,      /* Don't clear any notification bits on entry. */
                     UINT32_MAX, /* Reset the notification value to 0 on exit. */
                     &dbwTaskNotifications, /* Notified value pass out in
                                          dbwTaskNotifications. */
                     pdMS_TO_TICKS(MOTOR_CONTROLLER_PDU_PowerOnOff_Timeout_MS));  /* Timeout */

    if (rc == pdFALSE) {
        DEBUG_PRINT("Timed out waiting for mc on\n");
        return HAL_TIMEOUT;
    } else if (dbwTaskNotifications & (1<<NTFY_MCs_ON)) {
        DEBUG_PRINT("PDU has turned on MCs\n");
    } else {
        ERROR_PRINT("Got unexpected notification 0x%lX\n", dbwTaskNotifications);
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef turnOffMotorControllers() {
    uint32_t dbwTaskNotifications;

    // Request PDU to turn off motor controllers
    EM_Power_State_Request = EM_Power_State_Request_Off;
    sendCAN_VCU_EM_Power_State_Request();

    // Wait for PDU to turn off MCs
    BaseType_t rc = xTaskNotifyWait( 0x00,      /* Don't clear any notification bits on entry. */
                     UINT32_MAX, /* Reset the notification value to 0 on exit. */
                     &dbwTaskNotifications, /* Notified value pass out in
                                          dbwTaskNotifications. */
                     pdMS_TO_TICKS(MOTOR_CONTROLLER_PDU_PowerOnOff_Timeout_MS));  /* Timeout */

    if (rc == pdFALSE) {
        DEBUG_PRINT("Timed out waiting for mc off\n");
        return HAL_TIMEOUT;
    } else if (dbwTaskNotifications & (1<<NTFY_MCs_OFF)) {
        DEBUG_PRINT("PDU has turned off MCs\n");
    } else {
        ERROR_PRINT("Got unexpected notification 0x%lX\n", dbwTaskNotifications);
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef MotorStart()
{
    HAL_StatusTypeDef rc;

    DEBUG_PRINT("Starting motors\n");
    watchdogTaskChangeTimeout(DRIVE_BY_WIRE_TASK_ID,
                              pdMS_TO_TICKS(MOTOR_START_TASK_WATCHDOG_TIMEOUT_MS));

    rc = turnOnMotorController();
    if (rc != HAL_OK) {
        return rc;
    }

    rc = mcInit();
    if (rc != HAL_OK) {
        ERROR_PRINT("Failed to start motor controllers\n");
        return rc;
    }


    // Change back timeout
    watchdogTaskChangeTimeout(DRIVE_BY_WIRE_TASK_ID,
                              pdMS_TO_TICKS(DRIVE_BY_WIRE_WATCHDOG_TIMEOUT_MS));

    DEBUG_PRINT("MCs started up\n");
    return HAL_OK;
}

HAL_StatusTypeDef MotorStop()
{
    DEBUG_PRINT("Stopping motors\n");
    watchdogTaskChangeTimeout(DRIVE_BY_WIRE_TASK_ID, pdMS_TO_TICKS(2*MOTOR_STOP_TASK_WATCHDOG_TIMEOUT_MS));

    if (sendDisableMC() != HAL_OK) {
        ERROR_PRINT("Failed to shutdown motor controllers\n");
        return HAL_ERROR;
    }

    if (turnOffMotorControllers() != HAL_OK) {
        return HAL_ERROR;
    }

    // Change back timeout
    watchdogTaskChangeTimeout(DRIVE_BY_WIRE_TASK_ID,
                              pdMS_TO_TICKS(DRIVE_BY_WIRE_WATCHDOG_TIMEOUT_MS));
    return HAL_OK;
}

/*********************  DCU's functions **********************/
static uint32_t sendHvToggle(uint32_t event)
{
    const VCU_States_t current_state = fsmGetState(&VCUFsmHandle);
    VCU_States_t new_state = STATE_HV_Disable;

    switch (current_state)
    {
        case STATE_Failure_Fatal:
        {
            DEBUG_PRINT("Fault: Shouldn't toggle HV while faulted\r\n");
            sentFatalDTC = true;
            sendDTC_WARNING_VCU_SM_ERROR(2);
            new_state = STATE_Failure_Fatal;
            break;
        }
        case STATE_HV_Disable:  // fall through to STATE_EM_ENABLE
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
            sendDTC_WARNING_VCU_SM_ERROR(6);
            new_state = STATE_Failure_Fatal;
            break;
        }
    }

    return new_state;
}

// TODO: probably can be removed
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
    // TODO: double check if we still need this extra debouncing logic
    static TickType_t lastToggleTime = 0U;

    /*
    temporary check to prevent button from double clicking
    remove after 2024 michigan (the issue is probably the button itself??)
    */
    if (xTaskGetTickCount() - lastToggleTime < pdMS_TO_TICKS(EM_BUTTON_RATE_LIMIT_MS))
    {
        DEBUG_PRINT("Ignoring EM Toggle button event\n");
        return fsmGetState(&VCUFsmHandle);
    } else {
        lastToggleTime = xTaskGetTickCount();
    }

    const VCU_States_t current_state = fsmGetState(&VCUFsmHandle);

    if (current_state == STATE_HV_Disable || current_state == STATE_Failure_Fatal)
    {
        sentFatalDTC = true;
        DEBUG_PRINT("Cant EM while in state %d\r\n", current_state);

        sendDTC_WARNING_VCU_SM_ERROR(3);
        return current_state;
    }
    
    DEBUG_PRINT("Sending EM Toggle button event\n");
    // Should still log the EM toggle button event 
    if (sendEMToggleMsg() != HAL_OK)
    {
        ERROR_PRINT("Failed to send EM Toggle button event!\n");
        Error_Handler();
    } else {
        // Go to EM
        // TODO: double check this logic. Might be wrong cause this event causes a change in the state
        fsmSendEvent(&VCUFsmHandle, EV_EM_Toggle, portMAX_DELAY);
    }
    return current_state;
}

static uint32_t processHvState(uint32_t event)
{
    const VCU_States_t current_state = fsmGetState(&VCUFsmHandle);
    if(getHVState() == HV_Power_State_On)
    {
        if (current_state == STATE_HV_Enable || current_state == STATE_EM_Enable)
        {
            // Boards are probably out of sync. Power cycle should fix
            DEBUG_PRINT("FSM already at HV. FAULT\r\n");
            sentFatalDTC = true;
            sendDTC_WARNING_VCU_SM_ERROR(1);
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

static uint32_t fatalTransition(uint32_t event)
{
    DEBUG_PRINT("Fatal Event. Entering Fatal State\r\n");
    if (!sentFatalDTC)
    {
        sentFatalDTC = true;
        DEBUG_PRINT("Sending SM Fatal DTC\r\n");
        sendDTC_WARNING_VCU_SM_ERROR(0);
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
    toggle_TC();
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
        toggle_endurance_mode();
        DEBUG_PRINT("Endurance on\n");
    }
    else 
    {
        ENDURANCE_LED_OFF;
        DEBUG_PRINT("Endurance off\n");
    }
    return STATE_EM_Enable;
}

// /*
//  * A button press is considered valid if it is still low (active) after
//  * TIMER_WAIT_MS milliseconds.
//  */
// static void debounceTimerCallback(TimerHandle_t timer)
// {
//     GPIO_PinState pin_val;

//     switch (debouncingPin)
//     {
//         case HV_TOGGLE_BUTTON_PIN:
//             pin_val = HAL_GPIO_ReadPin(HV_TOGGLE_BUTTON_PORT,
//                     HV_TOGGLE_BUTTON_PIN);
//             break;

//         case EM_TOGGLE_BUTTON_PIN:
//             pin_val = HAL_GPIO_ReadPin(EM_TOGGLE_BUTTON_PORT,
//                     EM_TOGGLE_BUTTON_PIN);
//             break;
        
//         case TC_TOGGLE_BUTTON_PIN:
//             pin_val = HAL_GPIO_ReadPin(TC_TOGGLE_BUTTON_PORT,
//                     TC_TOGGLE_BUTTON_PIN);
//             break;
         
//         case ENDURANCE_TOGGLE_BUTTON_PIN:
//             pin_val = HAL_GPIO_ReadPin(ENDURANCE_TOGGLE_BUTTON_PORT,
//                     ENDURANCE_TOGGLE_BUTTON_PIN);
//             break;

//         default:
//             /* Shouldn't get here */ 
//             DEBUG_PRINT_ISR("Unknown pin specified to debounce\n");
//             pin_val = GPIO_PIN_SET;
//             break;
//     }


//     if (pin_val == GPIO_PIN_RESET)
//     {
//         switch (debouncingPin)
//         {
//             case HV_TOGGLE_BUTTON_PIN:
                // fsmSendEventISR(&VCUFsmHandle, EV_BTN_HV_Toggle);
//                 // DEBUG_PRINT_ISR("received HV button\r\n");
//                 break;

//             case EM_TOGGLE_BUTTON_PIN:
//                 // DEBUG_PRINT_ISR("received EM button\r\n");
//                 fsmSendEventISR(&VCUFsmHandle, EV_BTN_EM_Toggle);
//                 break;
            
//             case TC_TOGGLE_BUTTON_PIN:
//                 fsmSendEventISR(&VCUFsmHandle, EV_BTN_TC_Toggle);
//                 break;

//             case ENDURANCE_TOGGLE_BUTTON_PIN:
//                 fsmSendEventISR(&VCUFsmHandle, EV_BTN_Endurance_Mode_Toggle);
//                 break;

//             default:
//                 /* Shouldn't get here */
//                 DEBUG_PRINT_ISR("Unknown pin specified to debounce\n");
//                 break;
//         }

//     }

//     alreadyDebouncing = false;
// }

// void HAL_GPIO_EXTI_Callback(uint16_t pin)
// {
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//     if (alreadyDebouncing)
//     {
//         /* Already debouncing, do nothing with this interrupt */
//         return;
//     }
//     alreadyDebouncing = true;

//     switch (pin)
//     {
//         case HV_TOGGLE_BUTTON_PIN:
//             debouncingPin = HV_TOGGLE_BUTTON_PIN;
//             break;

//         case EM_TOGGLE_BUTTON_PIN:
//             debouncingPin = EM_TOGGLE_BUTTON_PIN;
//             break;
        
//         case TC_TOGGLE_BUTTON_PIN:
//             debouncingPin = TC_TOGGLE_BUTTON_PIN;
//             break;

//         case ENDURANCE_TOGGLE_BUTTON_PIN:
//             debouncingPin = ENDURANCE_TOGGLE_BUTTON_PIN;
//             break;

//         default:
//             /* Not a fatal error here, but report error and return */
//             DEBUG_PRINT_ISR("Unknown GPIO interrupted in ISR!\n");
//             return;
//             break;
//     }

//     xTimerStartFromISR(debounceTimer, &xHigherPriorityTaskWoken);
//     portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
// }

static void buzzerTimerCallback(TimerHandle_t timer)
{
    buzzerTimerStarted = false;
    BUZZER_DISABLE;
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
    return sendCAN_VCU_buttonEvents();
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
    return sendCAN_VCU_buttonEvents();
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
    return sendCAN_VCU_buttonEvents();
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
    return sendCAN_VCU_buttonEvents();
}