#ifndef __DRIVE_BY_WIRE_H
#define __DRIVE_BY_WIRE_H
#include "stm32f7xx_hal.h"
#include "state_machine.h"

#define MIN_BRAKE_PRESSURE 50 // TODO: Set this to a reasonable value

typedef enum VCU_States_t {
    STATE_Self_Check = 0,
    STATE_EM_Disable = 1,
    STATE_EM_Enable = 2,
    STATE_Failure_Fatal = 3,
    STATE_ANY, // Must be the last state
} VCU_States_t;

typedef enum VCU_Events_t {
    EV_Init = 0,
    EV_EM_Toggle,
    EV_Bps_Fail,
    EV_Hv_Disable,
    EV_Brake_Pressure_Fault,
    EV_DCU_Can_Timeout,
    EV_Throttle_Failure,
    EV_Throttle_Poll,
    EV_ANY, // Must be the last event
} VCU_Events_t;

extern FSM_Handle_Struct fsmHandle;

HAL_StatusTypeDef driveByWireInit(void);
HAL_StatusTypeDef startDriveByWire();
void driveByWireTask(void *pvParameters);
#endif // __DRIVE_BY_WIRE_H
