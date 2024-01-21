#ifndef __DRIVE_BY_WIRE_H
#define __DRIVE_BY_WIRE_H
#include "stm32f7xx_hal.h"
#include "state_machine.h"


typedef enum Main_PDU_States_t {
    STATE_Boards_Off = 0,
    STATE_Boards_On,
    STATE_Motors_On,
    STATE_Warning_Critical,
    STATE_Critical_Failure,
    STATE_ANY, // Must be the last state
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
    EV_Init = 0,
    EV_HV_CriticalFailure,
    EV_CriticalDelayElapsed,
    EV_EM_Enable,
    EV_EM_Disable,
    EV_ANY, // Must be the last event
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
