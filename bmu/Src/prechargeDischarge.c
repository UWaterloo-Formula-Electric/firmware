/**
  *****************************************************************************
  * @file    prechargeDischarge.c
  * @author  Richard Matthews
  * @brief   Controls the precharge and dishcarge procedures of the HV bus
  * @details The precharge and discharge procedure is repsonsible for safely
  * connecting and disconnecting the HV battery pack from the motor
  * controllers. It is controlled by the precharge discharge task (pcdcTask).
  * The main control state machine initiates precharge and discharge by sending
  * a notification to the pcdcTask, and discharge is initiated by the state
  * machine in the case of errors.
  *
  *****************************************************************************
  */

#include "prechargeDischarge.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "math.h"
#include "bmu_can.h"
#include "bmu_dtc.h"
#include "batteries.h"

/** Define this to enable contactor control, otherwise PCDC will always
 *  return successful.
 */
#define ENABLE_PRECHARGE_DISCHARGE

/**
 * Return types for precharge and discharge functions
 */
typedef enum Precharge_Discharge_Return_t {
    PCDC_DONE,    ///< Precharge/Discharge completed
    PCDC_STOPPED, ///< Precharge/Discharge stopped succesfully (as requested by task notification)
    PCDC_ERROR    ///< Error occured during Precharge/Discharge
} Precharge_Discharge_Return_t;

/**
 * Type of precharge to perform. This changes the checks performed during
 * precharge, due to different capacitance of motor controllers and chargers.
 */
typedef enum Precharge_Type_t {
    PC_MotorControllers, ///< Precharge motor controllers
    PC_Charger,          ///< Precharge charger
    PC_NumTypes,         ///< Must be defined last to accurately count enum
} Precharge_Type_t;


Precharge_Discharge_Return_t discharge();

HAL_StatusTypeDef pcdcInit()
{
    DEBUG_PRINT("PCDC Init\n");
    return HAL_OK;
}

/**
 * Enum to translate between contactor open/closed and GPIO pin state
 */
typedef enum ContactorState_t {
    CONTACTOR_CLOSED = GPIO_PIN_RESET,
    CONTACTOR_OPEN = GPIO_PIN_SET,
} ContactorState_t;

/**
 * @brief Control negative contactor
 *
 * @param state The state to set contactor to
 */
void setNegContactor(ContactorState_t state)
{
    DEBUG_PRINT("%s negative contactor\n", state==CONTACTOR_CLOSED?"Closing":"Opening");
    if (state==CONTACTOR_CLOSED) CONT_NEG_CLOSE;
    else if (state == CONTACTOR_OPEN) CONT_NEG_OPEN;
}

/**
 * @brief Control positive contactor
 *
 * @param state The state to set contactor to
 */
void setPosContactor(ContactorState_t state)
{
    DEBUG_PRINT("%s positive contactor\n", state==CONTACTOR_CLOSED?"Closing":"Opening");

    if (state==CONTACTOR_CLOSED) CONT_POS_CLOSE;
    else if (state == CONTACTOR_OPEN) CONT_POS_OPEN;
}

/**
 * @brief Control precharge discharge contactor
 *
 * @param state The state to set contactor to. Note for PCDC contactor closed
 * means precharge mode, open means discharge mode
 */
void setPrechargeContactor(ContactorState_t state)
{
    DEBUG_PRINT("%s precharge contactor\n", state==CONTACTOR_CLOSED?"Closing":"Opening");

    if (state==CONTACTOR_CLOSED) PCDC_PC;
    else if (state == CONTACTOR_OPEN) PCDC_DC;
}

/**
 * @brief Opens all contactors (safe state)
 */
void openAllContactors()
{
    setPosContactor(CONTACTOR_OPEN);
    setNegContactor(CONTACTOR_OPEN);
    setPrechargeContactor(CONTACTOR_OPEN);
}


/**
 * @brief Delay measure period, and interrupt delay if receive notification
 *
 * @param MeasurePeriod The period to delay for
 * @param NotificationOut The returned notification value
 *
 */
#define WAIT_FOR_NEXT_MEASURE_OR_STOP(MeasurePeriod, NotificationOut) \
    do { \
        (NotificationOut) = 0; /* Set to zero, in case no notifaction received */ \
        xTaskNotifyWait( 0x00,       /* Don't clear any notification bits on entry. */ \
                         UINT32_MAX, /* Reset the notification value to 0 on exit. */ \
                         &(NotificationOut), /* Notified value passed out */ \
                         pdMS_TO_TICKS((MeasurePeriod))); /* Timeout */ \
    } while(0)


/**
 * @brief Update HV Bus measurements
 *
 * @param[out] VBus pointer to float to store HV Bus voltage measurement in
 * volts
 * @param[out] VBatt pointer to float to store HV Battery voltage measurement in
 * volts
 * @param[out] IBus pointer to float to store HV Bus current measurement in
 * amps
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef updateMeasurements(float *VBus, float *VBatt, float *IBus)
{
    if (getVBatt(VBatt) != HAL_OK) {
        return HAL_ERROR;
    }
    if (getVBus(VBus) != HAL_OK) {
        return HAL_ERROR;
    }
    if (getIBus(IBus) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

// Set by CLI to modify precharge so it succeeds on HITL
bool HITL_Precharge_Mode = false;
float HITL_VPACK = 0;

// PC_MotorControllers precharges into motor controllers, so assumes a high
// capacitance and therefore precharge current
// PC_Charger precharges into the charger, so assumes a low capacitance and
// therefore negligible current
/**
 * @brief Perform precharge. If succesful, bus voltage will equal battery
 * voltage.
 *
 * @param prechargeType @ref PC_MotorControllers precharges into motor controllers,
 * so assumes a high capacitance and therefore precharge current. @ref PC_Charger
 * precharges into the charger, so assumes a low capacitance and therefore
 * negligible current
 *
 * @return @ref Precharge_Discharge_Return_t
 */
Precharge_Discharge_Return_t precharge(Precharge_Type_t prechargeType)
{
    float VBatt, VBus, IBus;
    uint32_t dbwTaskNotifications;
    DEBUG_PRINT("precharge type %d\n", prechargeType);
    if (prechargeType >= PC_NumTypes) {
        ERROR_PRINT("Invalid precharge type %d\n", prechargeType);
        return PCDC_ERROR;
    }

    float packVoltage;
    if (HITL_Precharge_Mode) {
        packVoltage = HITL_VPACK;
    } else {
        // Used to store the pack voltage before precharging (STEP 2), so we know
        // when we are done precharging
        // For step 1 and 2, get the value from the ams boards
        if (getPackVoltage(&packVoltage) != HAL_OK) {
            ERROR_PRINT("Failed to get pack voltage for precharge from queue\n");
            return PCDC_ERROR;
        }
        DEBUG_PRINT("pack voltage %f\n", packVoltage);
    }

    PrechargeState = 0; 
    sendCAN_PrechargeState();
    /*
     * Step 1:
     * IShunt == 0
     * VBUS = 0
     * VBATT = VPACK / 2
     */
    setPrechargeContactor(CONTACTOR_OPEN);
    setPosContactor(CONTACTOR_OPEN);
    setNegContactor(CONTACTOR_OPEN);

    // Delay for contactors to open and measurements to stabilize
    // Should already be the case for this step, but just in case
    WAIT_FOR_NEXT_MEASURE_OR_STOP(PRECHARGE_STEP_1_WAIT_TIME_MS,
                                  dbwTaskNotifications);
    if (dbwTaskNotifications & (1<<STOP_NOTIFICATION)) {
        DEBUG_PRINT("Precharge Stopped\n");
        return PCDC_STOPPED;
    }

    if (updateMeasurements(&VBus, &VBatt, &IBus) != HAL_OK) {
        DEBUG_PRINT("Update falied\r\n");
        return PCDC_ERROR;
    }

    if (VBus > packVoltage * PRECHARGE_STEP_1_VBUS_MAX_PERCENT_VPACK)
    {
        DEBUG_PRINT("Already precharged, discharging\n");
        if (discharge() != PCDC_DONE) {
            return PCDC_ERROR;
        }
    }

    DEBUG_PRINT("PC Step 1\n");
    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    if (VBus > packVoltage * PRECHARGE_STEP_1_VBUS_MAX_PERCENT_VPACK)
    {
        ERROR_PRINT("ERROR: VBUS %f > %f\n", VBus,
                    packVoltage * PRECHARGE_STEP_1_VBUS_MAX_PERCENT_VPACK);
        return PCDC_ERROR;
    }

    ERROR_PRINT("INFO: IBus %f\n", IBus);
    // if (IBus > PRECHARGE_STEP_1_CURRENT_MAX) {
    //     ERROR_PRINT("ERROR: IBus %f > %f\n", IBus, PRECHARGE_STEP_1_CURRENT_MAX);
    //     return PCDC_ERROR;
    // }

    PrechargeState = 1; 
    sendCAN_PrechargeState();
    /*
     * Step 2:
     * IShunt == 0
     * VBUS = 0
     * VBATT = 0.29*VPACK
     * Close Precharge contactor
     */
    setPrechargeContactor(CONTACTOR_CLOSED);
    setPosContactor(CONTACTOR_OPEN);
    setNegContactor(CONTACTOR_OPEN);

    uint32_t startTickCount = xTaskGetTickCount();
    // do {
    //     // Delay for contactors to close and measurements to stabilize
    //     WAIT_FOR_NEXT_MEASURE_OR_STOP(PRECHARGE_STEP_2_WAIT_TIME_MS,
    //                                   dbwTaskNotifications);
    //     if (dbwTaskNotifications & (1<<STOP_NOTIFICATION)) {
    //         DEBUG_PRINT("Precharge Stopped\n");
    //         return PCDC_STOPPED;
    //     }

    //     if (updateMeasurements(&VBus, &VBatt, &IBus) != HAL_OK) {
    //         return PCDC_ERROR;
    //     }

    //     if (xTaskGetTickCount() - startTickCount > PRECHARGE_STEP_2_TIMEOUT) {
    //         ERROR_PRINT("Precharge step 2 timed out waiting for IBus to zero\n");
    //         ERROR_PRINT("INFO: VBUS %f\n", VBus);
    //         ERROR_PRINT("INFO: VBatt %f\n", VBatt);
    //         ERROR_PRINT("INFO: IBus %f\n", IBus);
    //         return PCDC_ERROR;
    //     }
    // } while (IBus > PRECHARGE_STEP_2_CURRENT_MAX);

    DEBUG_PRINT("PC Step 2\n");
    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    
    //Due to IMD bias
    if (VBus > packVoltage * PRECHARGE_STEP_2_VBUS_MAX_PERCENT_VPACK)
    {
        ERROR_PRINT("ERROR: VBUS %f > %f\n", VBus,
                    packVoltage * PRECHARGE_STEP_2_VBUS_MAX_PERCENT_VPACK);
        return PCDC_ERROR;
    }
    ERROR_PRINT("INFO: VBatt %f\n", VBatt);
    if (VBatt > packVoltage*PRECHARGE_STEP_2_PERCENT_VBATT_MAX) {
        ERROR_PRINT("ERROR: VBatt %f > %f\n", VBatt, packVoltage*PRECHARGE_STEP_2_PERCENT_VBATT_MAX);
        return PCDC_ERROR;
    }
    ERROR_PRINT("INFO: IBus %f\n", IBus);

    PrechargeState = 2; 
    sendCAN_PrechargeState();
    /*
     * Step 3:
     * IShunt == 0
     * VBUS = 0
     * VBATT = VPACK / 2
     * Precharge open, neg contactor close, pos contactor open
     */

    // !!Need to open precharge before closing NEG contactor!!
    setPrechargeContactor(CONTACTOR_OPEN);
    setNegContactor(CONTACTOR_CLOSED);
    setPosContactor(CONTACTOR_OPEN);

    // Delay for contactors to close and measurements to stabilize
    WAIT_FOR_NEXT_MEASURE_OR_STOP(PRECHARGE_STEP_3_WAIT_TIME_MS,
                                  dbwTaskNotifications);
    if (dbwTaskNotifications & (1<<STOP_NOTIFICATION)) {
        DEBUG_PRINT("Precharge Stopped\n");
        return PCDC_STOPPED;
    }

    if (updateMeasurements(&VBus, &VBatt, &IBus) != HAL_OK) {
        return PCDC_ERROR;
    }

    DEBUG_PRINT("PC Step 3\n");
    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    ERROR_PRINT("INFO: VBATT %f\n", VBatt);
    if (VBus > packVoltage * PRECHARGE_STEP_3_VBUS_MAX_PERCENT_VPACK)
    {
        ERROR_PRINT("ERROR: VBUS %f > %f\n", VBus,
                    packVoltage * PRECHARGE_STEP_3_VBUS_MAX_PERCENT_VPACK);
        return PCDC_ERROR;
    }
    ERROR_PRINT("INFO: VBatt %f\n", VBatt);
    if (VBatt < packVoltage * PRECHARGE_STEP_3_VBATT_MIN_PERCENT_VPACK) {
        ERROR_PRINT("ERROR: VBatt %f < %f\n", VBatt,
                    packVoltage * PRECHARGE_STEP_3_VBATT_MIN_PERCENT_VPACK);
        return PCDC_ERROR;
    }
    // ERROR_PRINT("INFO: IBus %f\n", IBus);
    // if (IBus > PRECHARGE_STEP_3_CURRENT_MAX) {
    //     ERROR_PRINT("ERROR: IBus %f > %f\n", IBus, PRECHARGE_STEP_3_CURRENT_MAX);
    //     return PCDC_ERROR;
    // }

    PrechargeState = 3; 
    sendCAN_PrechargeState();
    /*
     * Step 4:
     * IShunt >= 1
     * VBUS = 0
     * VBATT = VPACK
     * precharge closed, neg contactor closed, pos contactor open
     */

    DEBUG_PRINT("PC Step 4\n");

    float maxIBus = 0;
    DEBUG_PRINT("VPack %f\r\nTick, VBUS, VBATT, IBUS\n", packVoltage);
    startTickCount = xTaskGetTickCount();

    setPrechargeContactor(CONTACTOR_CLOSED);
    setNegContactor(CONTACTOR_CLOSED);
    setPosContactor(CONTACTOR_OPEN);

    do {
        if (updateMeasurements(&VBus, &VBatt, &IBus) != HAL_OK) {
            return PCDC_ERROR;
        }

        ERROR_PRINT("%lu, %f, %f, %f\r\n", xTaskGetTickCount(), VBus, VBatt, IBus);
        if (IBus > maxIBus) {
            maxIBus = IBus;
        }
        WAIT_FOR_NEXT_MEASURE_OR_STOP(PRECHARGE_STEP_4_CURRENT_MEASURE_PERIOD_MS,
                                      dbwTaskNotifications);
        if (xTaskGetTickCount() - startTickCount > PRECHARGE_STEP_4_TIMEOUT) {
            ERROR_PRINT("Precharge timed out\n");
            ERROR_PRINT("INFO: VBUS %f\n", VBus);
            ERROR_PRINT("INFO: VBatt %f\n", VBatt);
            ERROR_PRINT("INFO: IBus %f\n", IBus);
            break;
        }
    } while (VBus < (packVoltage*PRECHARGE_STEP_4_COMPLETE_PERCENT_VPACK));

    uint32_t prechargeTime = xTaskGetTickCount() - startTickCount;
    DEBUG_PRINT("Precharge took %lu ticks\n", prechargeTime);
    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    ERROR_PRINT("INFO: VBatt %f\n", VBatt);
    ERROR_PRINT("INFO: IBus %f\n", IBus);

    if (prechargeType == PC_MotorControllers) {
        float minPrechargeCurrent = (packVoltage) / PRECHARGE_RESISTOR_OHMS;
        minPrechargeCurrent *= MIN_PRECHARGE_PERCENT_IDEAL_CURRENT;
        DEBUG_PRINT("Info: Max IBus: %f, needed %f\n", maxIBus, minPrechargeCurrent);
        if (!HITL_Precharge_Mode) {
            if (maxIBus < minPrechargeCurrent) {
            	ERROR_PRINT("Failed Step 4\n");
                ERROR_PRINT("Didn't detect precharge current!\n");
                ERROR_PRINT("Max IBus: %f, needed %f\n", maxIBus, minPrechargeCurrent);
                return PCDC_ERROR;
            }
        }
    }

    PrechargeState = 4; 
    sendCAN_PrechargeState();
    /*
     * Step 5:
     * IShunt has spike due to closing pos contactor
     */


    DEBUG_PRINT("PC Step 5\n");

    float step4EndVBus = VBus;
    DEBUG_PRINT("Step4EndVBus: %f\n", step4EndVBus);

    setPrechargeContactor(CONTACTOR_CLOSED);
    setNegContactor(CONTACTOR_CLOSED);
    setPosContactor(CONTACTOR_CLOSED);

    startTickCount = xTaskGetTickCount();
    DEBUG_PRINT("Tick, VBUS, VBATT, IBUS\n");
    do {
        if (updateMeasurements(&VBus, &VBatt, &IBus) != HAL_OK) {
            return PCDC_ERROR;
        }
        ERROR_PRINT("%lu,", xTaskGetTickCount());
        ERROR_PRINT("%f,", VBus);
        ERROR_PRINT("%f,", VBatt);
        ERROR_PRINT("%f\n", IBus);
        if (IBus > maxIBus) {
            maxIBus = IBus;
        }
        WAIT_FOR_NEXT_MEASURE_OR_STOP(PRECHARGE_STEP_5_CURRENT_MEASURE_PERIOD_MS,
                                      dbwTaskNotifications);
        if (xTaskGetTickCount() - startTickCount > PRECHARGE_STEP_5_TIMEOUT) {
            ERROR_PRINT("Precharge timed out\n");
            ERROR_PRINT("INFO: VBUS %f\n", VBus);
            ERROR_PRINT("INFO: VBatt %f\n", VBatt);
            ERROR_PRINT("INFO: IBus %f\n", IBus);
            break;
        }
    } while (VBus < (packVoltage*PRECHARGE_STEP_5_COMPLETE_PERCENT_VPACK));

    DEBUG_PRINT("Precharge step 5 took %lu ticks\n", xTaskGetTickCount() - startTickCount);

    if (PC_MotorControllers) {
        float minIBusSpike = (packVoltage - step4EndVBus) / PRECHARGE_RESISTOR_OHMS;
        minIBusSpike *= PRECHARGE_STEP_5_PERCENT_IDEAL_CURRENT_REQUIRED;

        DEBUG_PRINT("Info: Max IBus: %f, needed %f\n", maxIBus, minIBusSpike);
        if (!HITL_Precharge_Mode) {
            if (maxIBus < minIBusSpike) {
                ERROR_PRINT("IBus %f, required spike %f\n", maxIBus, minIBusSpike);
                return PCDC_ERROR;
            }
        }
    }

    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    ERROR_PRINT("INFO: VBatt %f\n", VBatt);
    ERROR_PRINT("INFO: IBus %f\n", IBus);

    DEBUG_PRINT("Finished Precharge\n");

    PrechargeState = 5; 
    sendCAN_PrechargeState();
    return PCDC_DONE;
}

/**
 * @brief Discharges HV Bus. Opens contactors which connects in discharge
 * resistor to discharge bus voltage. This function sends out a warning DTC,
 * then waits (with timeout) for bus current to go to zero before opening
 * contactors. This attempts to avoid opening contactors under load which can
 * cause arcing/welding of contactors (very bad).
 *
 * @return @ref Precharge_Discharge_Return_t
 */
Precharge_Discharge_Return_t discharge()
{
    sendDTC_WARNING_CONTACTOR_OPEN_IMPENDING();
    DEBUG_PRINT("Discharge start, waiting for zero current\n");
    uint32_t startTickVal = xTaskGetTickCount();
    float IBus, VBus, VBatt;

    //Reset the can logs signals
    DischargeState = 0;
    VBus_Data = 0;
    VBatt_Data = 0;
    IBus_Data = 0;

    do {
        if (getIBus(&IBus) != HAL_OK) {
            break;
        }

        if (xTaskGetTickCount() - startTickVal >
            pdMS_TO_TICKS(CONTACTOR_OPEN_ZERO_CURRENT_TIMEOUT_MS))
        {
            ERROR_PRINT("Timed out waiting for zero current before opening contactors\n");
            break;
        }
        IBus_Data = IBus;
        sendCAN_Discharge_Data();
        vTaskDelay(1);
    } while (IBus > ZERO_CURRENT_MAX_AMPS);

    //Publish Discharge state for can logs
    DischargeState = 1;
    VBus_Data = 0;
    VBatt_Data = 0;
    IBus_Data = 0;
    sendCAN_Discharge_Data();

    DEBUG_PRINT("Opening contactors\n");
    openAllContactors();

    DEBUG_PRINT("Tick, VBUS, VBATT, IBUS\n");
    uint32_t startTickCount = xTaskGetTickCount();
    do {
        if (updateMeasurements(&VBus, &VBatt, &IBus) != HAL_OK) {
            return PCDC_ERROR;
        }

        ERROR_PRINT("%lu,", xTaskGetTickCount());
        ERROR_PRINT("%f,", VBus);
        ERROR_PRINT("%f,", VBatt);
        ERROR_PRINT("%f\n", IBus);

        if (xTaskGetTickCount() - startTickCount > PRECHARGE_STEP_4_TIMEOUT) {
            ERROR_PRINT("Discharge timed out\n");
            ERROR_PRINT("INFO: VBUS %f\n", VBus);
            ERROR_PRINT("INFO: VBatt %f\n", VBatt);
            ERROR_PRINT("INFO: IBus %f\n", IBus);
            return PCDC_ERROR;
        }
        VBus_Data = VBus;
        VBatt_Data = VBatt;
        IBus_Data = IBus;
        sendCAN_Discharge_Data();
        vTaskDelay(pdMS_TO_TICKS(DISCHARGE_MEASURE_PERIOD_MS));
    } while (VBus > DISCHARGE_DONE_BUS_VOLTAGE);

    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    ERROR_PRINT("INFO: VBatt %f\n", VBatt);
    ERROR_PRINT("INFO: IBus %f\n", IBus);

    DEBUG_PRINT("Discharge done\n");

    VBus_Data = 0;
    VBatt_Data = 0;
    IBus_Data = 0;
    DischargeState = 2;
    sendCAN_Discharge_Data();
    return PCDC_DONE;
}

/**
 * @brief Task to control precharge and discharge. Receives notifications from
 * the control state machine to either precharge/discharge or stop precharge.
 *
 */
void pcdcTask(void *pvParameter)
{
    uint32_t dbwTaskNotifications;
    Precharge_Discharge_Return_t rc;

    if (pcdcInit() != HAL_OK) {
        Error_Handler();
    }

    while (1)
    {
        // wait for notification
        xTaskNotifyWait( 0x00,      /* Don't clear any notification bits on entry. */
                         UINT32_MAX, /* Reset the notification value to 0 on exit. */
                         &dbwTaskNotifications, /* Notified value pass out in
                                              dbwTaskNotifications. */
                         portMAX_DELAY);  /* Timeout */

        if (dbwTaskNotifications & (1<<STOP_NOTIFICATION)) {

            DEBUG_PRINT("Skipping precharge/discharge due to stop\n");

        } else if (dbwTaskNotifications & (1<<PRECHARGE_NOTIFICATION_CHARGER) ||
                   dbwTaskNotifications & (1<<PRECHARGE_NOTIFICATION_MOTOR_CONTROLLERS)) {
            DEBUG_PRINT("Starting precharge\n");

            if (dbwTaskNotifications & (1<<PRECHARGE_NOTIFICATION_MOTOR_CONTROLLERS)) {
#ifdef ENABLE_PRECHARGE_DISCHARGE
                rc = precharge(PC_MotorControllers);
#else
                rc = PCDC_DONE;
#endif
            } else if (dbwTaskNotifications & (1<<PRECHARGE_NOTIFICATION_CHARGER)) {
#ifdef ENABLE_PRECHARGE_DISCHARGE
                rc = precharge(PC_Charger);
#else
                rc = PCDC_DONE;
#endif
            } else {
                ERROR_PRINT("Unknown precharge type!");
                rc = PCDC_ERROR;
            }

            if (rc == PCDC_DONE) {
                DEBUG_PRINT("Precharge done\n");
                fsmSendEvent(&fsmHandle, EV_Precharge_Finished, portMAX_DELAY);
            } else if (rc == PCDC_ERROR) {
                DEBUG_PRINT("Precharge Error\n");
                PrechargeState = 6;
                sendCAN_PrechargeState();
#ifdef ENABLE_PRECHARGE_DISCHARGE
                discharge();
#endif
                fsmSendEvent(&fsmHandle, EV_PrechargeDischarge_Fail, portMAX_DELAY);
            } else {
                DEBUG_PRINT("Precharge Stopped\n");
#ifdef ENABLE_PRECHARGE_DISCHARGE
                discharge();
#endif
                // TODO: Is there anything to do if the precharge gets stopped?
            }

        } else if (dbwTaskNotifications & (1<<DISCHARGE_NOTIFICATION)) {
#ifdef ENABLE_PRECHARGE_DISCHARGE
            rc = discharge();
#else
            rc = PCDC_DONE;
#endif

            if (rc != PCDC_DONE) {
                ERROR_PRINT("Failed to discharge\n");
                openAllContactors();
                fsmSendEvent(&fsmHandle, EV_PrechargeDischarge_Fail, portMAX_DELAY);
            } else {
                fsmSendEvent(&fsmHandle, EV_Discharge_Finished, portMAX_DELAY);
            }

        } else {
            ERROR_PRINT("Unkown notification received 0x%lX\n", dbwTaskNotifications);
        }
    }
}

