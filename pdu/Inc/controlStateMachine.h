#ifndef __DRIVE_BY_WIRE_H
#define __DRIVE_BY_WIRE_H
#include "stm32f7xx_hal.h"
#include "state_machine.h"


typedef enum Main_PDU_States_t {
    MN_STATE_Boards_Off = 0,
    MN_STATE_Boards_On,
    MN_STATE_Motors_On,
    MN_STATE_Warning_Critical,
    MN_STATE_Critical_Failure,
    MN_STATE_ANY, // Must be the last state
} Main_PDU_States_t;

char PDU_Main_States_String[][25]={
    "Boards Off", 
    "Boards On", 
    "Warning Critical", 
    "Critical Failure",
    "Motors on",
    "Motors off"
};

typedef enum MAIN_PDU_Events_t {
    MN_EV_Init = 0,
    MN_EV_HV_CriticalFailure,
    MN_EV_CriticalDelayElapsed,
    MN_EV_EM_Enable,
    MN_EV_EM_Disable,
    MN_EV_ANY, // Must be the last event
} MAIN_PDU_Events_t;

extern FSM_Handle_Struct mainFsmHandle;

HAL_StatusTypeDef mainControlInit();
void mainControlTask(void *pvParameters);

// Watchdog Task IDs - not sure where else to put these
#define MAIN_CONTROL_TASK_ID 1
#define COOLING_TASK_ID 6
#define SENSOR_TASK_ID 4
#define POWER_TASK_ID 5

#endif // __DRIVE_BY_WIRE_H
