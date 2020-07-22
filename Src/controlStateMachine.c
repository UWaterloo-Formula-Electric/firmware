
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stdbool.h"

#include "bsp.h"
#include "debug.h"
#include "DCU_can.h"
#include "userCan.h"
#include "watchdog.h"
#include "canReceive.h"
#include "controlStateMachine.h"

#define EM_BUTTON_DEBOUNCE_MS 200
#define HV_BUTTON_DEBOUNCE_MS 200

#define MAIN_TASK_ID 1

// The task just waits for buttons, so doesn't really need a period
// Put one so it can work with the watchdog though
// This is just the amount of time it will wait for events
// It then refreshes the watchdog, and either handles events or goes back to
// waiting
#define MAIN_TASK_PERIOD 100
extern osThreadId mainTaskHandle;
#define BUZZER_LENGTH_MS 1000
TimerHandle_t buzzerSoundTimer;
void buzzerTimerCallback(TimerHandle_t timer);
bool buzzerTimerStarted = false;


uint32_t toggleOnHV(uint32_t event);
uint32_t toggleOnEM(uint32_t event);
uint32_t hvControl(uint32_t event);
uint32_t emControl(uint32_t event);
HAL_StatusTypeDef fsmInit();
void mainTaskFunction(void const * argument);

Transition_t transitions[] = {
    {STATE_HV_Disable,EV_HV_Toggle, &toggleOnHV},
    {STATE_HV_Toggle, EV_CAN_Recieve, &hvControl},
    {STATE_HV_Enable, EV_EM_Toggle, &toggleOnEM},
    {STATE_EM_Toggle, EV_CAN_Recieve,&emControl},
    {STATE_EM_Enable, EV_EM_Toggle,},
    {STATE_HV_Enable, EV_HV_Toggle,}
}
uint32_t toggleOnHV(uint32_t event){
    DEBUG_PRINT("Sending hv changed\n");
    sendCAN_DCU_buttonEvents();
    return STATE_HV_Toggle;
}
uint32_t toggleOnEM(uint32_t event){
    DEBUG_PRINT("Sending em changed\n");
    sendCAN_DCU_buttonEvents();
    if (!buzzerTimerStarted) {
        if (xTimerStart(buzzerSoundTimer, 100) != pdPASS) {
            ERROR_PRINT("Failed to start buzzer timer\n");
            continue;
        }
        buzzerTimerStarted = true;
        BUZZER_ON
    }

    return STATE_EM_Toggle;
}
uint32_t hvControl(uint32_t event){
    if(getHVState() == HV_Power_State_On){
        DEBUG_PRINT("HV Enabled");
        return STATE_HV_Enable;
    }
    else{
        DEBUG_PRINT("HV Disabled");
        return STATE_HV_Disable;
    }
}
uint32_t emControl(uint32_t event){
    if(getEMState() == EM_Power_State_On){
        DEBUG_PRINT("EM Enabled");
        return STATE_EM_Enable;
    }
    else{
        DEBUG_PRINT("EM Disabled");
        return STATE_HV_Enable;
    }
}
HAL_StatusTypeDef fsmInit(){
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
    if (fsmInit(STATE_HV_Disable, &init, &DCUFsmHandle) != HAL_OK) {
        ERROR_PRINT("Failed to init DCU fsm\n");
        return HAL_ERROR;
    }

    DEBUG_PRINT("Init DCU fsm\n");
    return HAL_OK;

}

void mainTaskFunction(void const * argument){
    DEBUG_PRINT("Starting up!!\n");
    if (canStart(&CAN_HANDLE) != HAL_OK) {
        ERROR_PRINT("Failed to start can\n");
        Error_Handler();
    }
    buzzerSoundTimer = xTimerCreate("BuzzerTimer",
                                       pdMS_TO_TICKS(BUZZER_LENGTH_MS),
                                       pdFALSE /* Auto Reload */,
                                       0,
                                       buzzerTimerCallback);

    if (buzzerSoundTimer == NULL) {
        ERROR_PRINT("Failed to create throttle timer\n");
        Error_Handler();
    }


    if (registerTaskToWatch(MAIN_TASK_ID, 5*pdMS_TO_TICKS(MAIN_TASK_PERIOD), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register main task with watchdog!\n");
        Error_Handler();
    }
    fsmTaskFunction(&DCUFsmHandle);
    for(;;);
}
void buzzerTimerCallback(TimerHandle_t timer)
{
    buzzerTimerStarted = false;
    BUZZER_OFF
}
