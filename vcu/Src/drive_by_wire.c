#include "drive_by_wire.h"
#include "stm32f7xx_hal.h"
#include "drive_by_wire_mock.h"
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

FSM_Handle_Struct fsmHandle;

uint32_t runSelftTests(uint32_t event);
uint32_t EM_Enable(uint32_t event);
uint32_t EM_Fault(uint32_t event);
uint32_t EM_Update_Throttle(uint32_t event);
uint32_t doNothing(uint32_t event);
uint32_t DefaultTransition(uint32_t event);

void throttleTimerCallback(TimerHandle_t timer);
HAL_StatusTypeDef MotorStart();
HAL_StatusTypeDef MotorStop();

extern osThreadId throttlePollingHandle;


Transition_t transitions[] = {
    { STATE_Self_Check, EV_Init, &runSelftTests },
    { STATE_EM_Disable, EV_EM_Toggle, &EM_Enable },
    { STATE_EM_Disable, EV_Bps_Fail, &EM_Fault },
    { STATE_EM_Disable, EV_Hv_Disable, &EM_Fault },
    { STATE_EM_Disable, EV_Brake_Pressure_Fault, &EM_Fault },
    { STATE_EM_Disable, EV_DCU_Can_Timeout, &EM_Fault },
    { STATE_EM_Disable, EV_Throttle_Failure, &EM_Fault },
    { STATE_EM_Enable, EV_Bps_Fail, &EM_Fault },
    { STATE_EM_Enable, EV_Hv_Disable, &EM_Fault },
    { STATE_EM_Enable, EV_Brake_Pressure_Fault, &EM_Fault },
    { STATE_EM_Enable, EV_DCU_Can_Timeout, &EM_Fault },
    { STATE_EM_Enable, EV_Throttle_Failure, &EM_Fault },
    { STATE_EM_Enable, EV_EM_Toggle, &EM_Fault },
    { STATE_EM_Enable, EV_Inverter_Fault, &EM_Fault },
    { STATE_Failure_Fatal, EV_ANY, &doNothing },
    { STATE_ANY, EV_Fatal, &EM_Fault },
    { STATE_ANY, EV_ANY, &DefaultTransition}
};

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
    if (fsmInit(STATE_Self_Check, &init, &fsmHandle) != HAL_OK) {
        ERROR_PRINT("Failed to init drive by wire fsm\n");
        return HAL_ERROR;
    }

    if (registerTaskToWatch(DRIVE_BY_WIRE_TASK_ID, pdMS_TO_TICKS(DRIVE_BY_WIRE_WATCHDOG_TIMEOUT_MS),
                            true, &fsmHandle) != HAL_OK)
    {
        return HAL_ERROR;
    }

    DEBUG_PRINT("Init drive by wire\n");
    return HAL_OK;
}

void driveByWireTask(void *pvParameters)
{
    // Pre send EV_INIT to kick off self tests
    startDriveByWire();

    if (canStart(&CAN_HANDLE) != HAL_OK) {
        Error_Handler();
    }

    fsmTaskFunction(&fsmHandle);

    for(;;); // Shouldn't reach here
}

HAL_StatusTypeDef startDriveByWire()
{
    return fsmSendEvent(&fsmHandle, EV_Init, portMAX_DELAY /* timeout */); // Force run of self checks
}

uint32_t runSelftTests(uint32_t event)
{
    // TODO: Run some tests
    
    if (brakeAndThrottleStart() != HAL_OK)
    {
        sendDTC_CRITICAL_Throttle_Failure(3);
        return EM_Fault(EV_Throttle_Failure);
    }

    return STATE_EM_Disable;
}

uint32_t EM_Enable(uint32_t event)
{
    // bool bpsState = checkBPSState();
    // bool hvEnable = getHvEnableState();
    // float brakePressure = getBrakePressure();
    uint32_t state = STATE_EM_Enable;

    // if (!bpsState) {
    //     DEBUG_PRINT("Failed to em enable, bps fault\n");
    //     sendDTC_WARNING_EM_ENABLE_FAILED(0);
    //     state = STATE_EM_Disable;
    // } else if (!(brakePressure > MIN_BRAKE_PRESSURE)) {
    //     DEBUG_PRINT("Failed to em enable, brake pressure low (%f)\n", brakePressure);
    //     sendDTC_WARNING_EM_ENABLE_FAILED(1);
    //     state = STATE_EM_Disable;
    // } else if (!(throttleIsZero())) {
    //     DEBUG_PRINT("Failed to em enable, non-zero throttle\n");
    //     sendDTC_WARNING_EM_ENABLE_FAILED(2);
    //     state = STATE_EM_Disable;
    // } else if (!isBrakePressed()) {
    //     DEBUG_PRINT("Failed to em enable, brake is not pressed\n");
    //     sendDTC_WARNING_EM_ENABLE_FAILED(3);
    //     state = STATE_EM_Disable;
    // } else if (!hvEnable) {
    //     sendDTC_WARNING_EM_ENABLE_FAILED(4);
    //     DEBUG_PRINT("Failed to em enable, not HV enabled\n");
    //     state = STATE_EM_Disable;
    // }

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
            state = STATE_EM_Disable;
        } else {
            sendDTC_FATAL_EM_ENABLE_FAILED(6);
            state = STATE_Failure_Fatal;
        }
    }

	endurance_mode_EM_callback();

    EM_State = (state == STATE_EM_Enable)?EM_State_On:EM_State_Off;
    sendCAN_VCU_EM_State();

    xTaskNotifyGive(throttlePollingHandle); // shouldn't we only notify if it was successful?

    return state;
}

uint32_t EM_Fault(uint32_t event)
{
    int newState = STATE_Failure_Fatal;
    int currentState = fsmGetState(&fsmHandle);
    
    EMFaultEvent = event;
    sendCAN_VCU_EM_Fault();

    if (fsmGetState(&fsmHandle) == STATE_Failure_Fatal) {
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
        case EV_DCU_Can_Timeout:
            {
                sendDTC_FATAL_DCU_CAN_Timeout();
                DEBUG_PRINT("DCU CAN Timeout, trans to fatal failure\n");
                newState = STATE_Failure_Fatal;
            }
            break;
        case EV_Throttle_Failure:
            {
                DEBUG_PRINT("Throttle read failure, trans to fatal failure\n");
                newState = STATE_Failure_Fatal;
            }
            break;
        case EV_Hv_Disable:
            {
                if (currentState == STATE_EM_Disable) {
                    DEBUG_PRINT("HV Disable, staying in EM Disabled state\n");
                } else {
                    //disable TC
                    disable_TC();
                    DEBUG_PRINT("HV Disable, trans to EM Disabled\n");
                }
                newState = STATE_EM_Disable;
            }
            break;
        case EV_EM_Toggle:
            {
                DEBUG_PRINT("EM Toggle, trans to EM Disabled\n");
                //disable TC
                disable_TC();
                newState = STATE_EM_Disable;
            }
            break;
        case EV_Fatal:
            {
                DEBUG_PRINT("Received fatal event, trans to fatal failure\n");
                sendDTC_FATAL_VCU_F7_EV_FATAL();
                newState = STATE_Failure_Fatal;
            }
            break;
        case EV_Inverter_Fault:
            {
                uint64_t faults = getInverterFaultCode();
                sendDTC_CRITICAL_VCU_Inverter_Fault(faults);
                // Check RMS GUI or CAN message 'MC_Fault_Codes' for fault details
                DEBUG_PRINT("Inverter faulted, trans to fatal failure\n");
                newState = STATE_Failure_Fatal;
            }
        default:
            {
                sendDTC_FATAL_VCU_F7_EM_ENABLED_ERROR();
                DEBUG_PRINT("EM Enabled, unknown event %lu\n", event);
            }
            break;
    }

    if (fsmGetState(&fsmHandle) == STATE_EM_Enable) {
        if (MotorStop() != HAL_OK) {
            ERROR_PRINT("Failed to stop motors\n");
            newState = STATE_Failure_Fatal;
        }

        EM_State = EM_State_Off;
        sendCAN_VCU_EM_State();
    }

    return newState;
}

uint32_t DefaultTransition(uint32_t event)
{
    ERROR_PRINT("No transition function registered for state %lu, event %lu\n",
                fsmGetState(&fsmHandle), event);

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

    // vTaskDelay(pdMS_TO_TICKS(MC_STARTUP_TIME_MS)); // prob need to change
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
    watchdogTaskChangeTimeout(DRIVE_BY_WIRE_TASK_ID, pdMS_TO_TICKS(MOTOR_STOP_TASK_WATCHDOG_TIMEOUT_MS));

    if (mcDisable() != HAL_OK) {
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

uint32_t doNothing(uint32_t event)
{
    return fsmGetState(&fsmHandle);
}
