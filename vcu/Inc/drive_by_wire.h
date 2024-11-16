#ifndef __DRIVE_BY_WIRE_H
#define __DRIVE_BY_WIRE_H
#include "stm32f7xx_hal.h"
#include "state_machine.h"

// Throttle poll time is linked to brake timeout and implausibilty timeout
#define THROTTLE_POLL_TIME_MS 50

#define MOTOR_CONTROLLER_PDU_PowerOnOff_Timeout_MS 1000 // TODO: Change to good value
#define INVERTER_ON_TIMEOUT_MS       10000

// While the motors are starting, increase the watchdog timeout to allow
// delays to wait for motor controllers to start up
#define MOTOR_START_TASK_WATCHDOG_TIMEOUT_MS ((2 * INVERTER_ON_TIMEOUT_MS) + MOTOR_CONTROLLER_PDU_PowerOnOff_Timeout_MS + 1000)
#define MOTOR_STOP_TASK_WATCHDOG_TIMEOUT_MS (MOTOR_CONTROLLER_PDU_PowerOnOff_Timeout_MS + 1000)

#define DRIVE_BY_WIRE_WATCHDOG_TIMEOUT_MS 20

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
    "Failure Fatal"
};

typedef enum VCU_Events_t {
    EV_Init = 0,
    EV_EM_Toggle,
    EV_Bps_Fail,
    EV_Hv_Disable,
    EV_Brake_Pressure_Fault,
    EV_DCU_Can_Timeout,
    EV_Throttle_Failure,
    EV_Fatal,
    EV_Inverter_Fault,
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
HAL_StatusTypeDef MotorStop();
HAL_StatusTypeDef turnOnMotorController();
HAL_StatusTypeDef turnOffMotorControllers();
void driveByWireTask(void *pvParameters);
#endif // __DRIVE_BY_WIRE_H
