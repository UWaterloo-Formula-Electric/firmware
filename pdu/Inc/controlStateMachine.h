#ifndef __DRIVE_BY_WIRE_H
#define __DRIVE_BY_WIRE_H
#include "stm32f7xx_hal.h"
#include "state_machine.h"

#define lutLen(X) (sizeof(X) / sizeof((X)[0]))

typedef enum Main_PDU_States_t {
    MN_STATE_Boards_Off = 0,
    MN_STATE_Boards_On,
    MN_STATE_Warning_Critical,
    MN_STATE_LV_Shutting_Down,
    MN_STATE_Critical_Failure,
    MN_STATE_ANY, // Must be the last state
} Main_PDU_States_t;

char PDU_Main_States_String[][25]={
    "Boards Off", 
    "Boards On", 
    "Warning Critical", 
    "LV Shutting Down", 
    "Critical Failure", 
    "Any State"
};

typedef enum MAIN_PDU_Events_t {
    MN_EV_Init = 0,
    MN_EV_HV_CriticalFailure,
    MN_EV_CriticalDelayElapsed,
    MN_EV_LV_Cuttoff,
    MN_EV_LV_Shutdown,
    MN_EV_ANY, // Must be the last event
} MAIN_PDU_Events_t;

typedef enum MotorControl_PDU_States_t {
    MTR_STATE_Motors_Off = 0,
    MTR_STATE_Motors_On,
    MTR_STATE_Critical,
    MTR_STATE_ANY, // Must be the last state
} MotorControl_PDU_States_t;

char PDU_Motor_States_String[][20]={
    "Motors Off", 
    "Motors On", 
    "Critical", 
    "Any State"
};

typedef enum MotorControl_PDU_Events_t {
    MTR_EV_EM_ENABLE = 0,
    MTR_EV_EM_DISABLE,
    MTR_EV_Motor_Critical,
    MTR_EV_ANY, // Must be the last event
} MotorControl_PDU_Events_t;

typedef enum CoolingControl_PDU_States_t {
    COOL_STATE_OFF,
    COOL_STATE_WAIT,
    COOL_STATE_ON,
    COOL_STATE_HV_CRITICAL,
    COOL_STATE_LV_Cuttoff,
    COOL_STATE_ANY, // Must be the last state
} CoolingControl_PDU_States_t;

char PDU_Cool_States_String[][15]={
    "OFF", 
    "WAIT", 
    "ON", 
    "HV_CRITICAL", 
    "LV_Cutoff", 
    "Any State"
};

typedef enum CoolingControl_PDU_Events_t {
    COOL_EV_EM_ENABLE,
    COOL_EV_EM_DISABLE,
    COOL_EV_WAIT_ELAPSED,
    COOL_EV_OVERTEMP_WARNING,
    COOL_EV_Critical,
    COOL_EV_LV_Cuttoff,
    COOL_EV_ANY, // Must be the last event
} CoolingControl_PDU_Events_t;

extern FSM_Handle_Struct mainFsmHandle;
extern FSM_Handle_Struct motorFsmHandle;
extern FSM_Handle_Struct coolingFsmHandle;

HAL_StatusTypeDef initStateMachines();
void mainControlTask(void *pvParameters);
#endif // __DRIVE_BY_WIRE_H
