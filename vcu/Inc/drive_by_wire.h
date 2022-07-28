#ifndef __DRIVE_BY_WIRE_H
#define __DRIVE_BY_WIRE_H
#include "stm32f7xx_hal.h"
#include "state_machine.h"
#define lutLen(X) (sizeof(X) / sizeof((X)[0]))

// Throttle poll time is linked to brake timeout and implausibilty timeout
#define THROTTLE_POLL_TIME_MS 50

#define MOTOR_CONTROLLER_PDU_PowerOnOff_Timeout_MS 1000 // TODO: Change to good value
#define MC_STARTUP_TIME_MS           1000
#define INVERTER_ON_TIMEOUT_MS       10000

typedef enum VCU_States_t {
    STATE_Self_Check = 0,
    STATE_EM_Disable = 1,
    STATE_EM_Enable = 2,
    STATE_Failure_Fatal = 3,
    STATE_ANY, // Must be the last state
} VCU_States_t;

char VCU_States_String[][20]={
    "Self Check", 
    "Em Disable", 
    "Em Enable", 
    "Failure Fatal", 
    "State Any"
};

typedef enum VCU_Events_t {
    EV_Init = 0,
    EV_EM_Toggle,
    EV_Bps_Fail,
    EV_Hv_Disable,
    EV_Brake_Pressure_Fault,
    EV_DCU_Can_Timeout,
    EV_Throttle_Failure,
    EV_Throttle_Poll,
    EV_Fatal,
    EV_ANY, // Must be the last event
} VCU_Events_t;

// Bit numbers for drive by wire task notification bit fields
typedef enum DBW_Task_Notifications_t {
    NTFY_MCs_ON = 0,
    NTFY_MCs_OFF = 1,
} DBW_Task_Notifications_t;

extern FSM_Handle_Struct fsmHandle;

HAL_StatusTypeDef driveByWireInit(void);
HAL_StatusTypeDef startDriveByWire();
void driveByWireTask(void *pvParameters);
#endif // __DRIVE_BY_WIRE_H
