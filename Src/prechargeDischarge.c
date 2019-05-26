#include "prechargeDischarge.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "freertos.h"
#include "task.h"
#include "math.h"
#include "BMU_DTC.h"
#include "batteries.h"

HAL_StatusTypeDef pcdcInit()
{
    DEBUG_PRINT("PCDC Init\n");
    return HAL_OK;
}

HAL_StatusTypeDef getIBus(float *IBus)
{
    if (xQueuePeek(IBusQueue, IBus, 0) != pdTRUE) {
        ERROR_PRINT("Failed to receive IBus current from queue\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef getVBatt(float *VBatt)
{
    if (xQueuePeek(VBattQueue, VBatt, 0) != pdTRUE) {
        ERROR_PRINT("Failed to receive VBatt voltage from queue\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef getVBus(float * VBus)
{
    if (xQueuePeek(VBusQueue, VBus, 0) != pdTRUE) {
        ERROR_PRINT("Failed to receive VBus voltage from queue\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

typedef enum ContactorState_t {
    CONTACTOR_CLOSED = GPIO_PIN_RESET,
    CONTACTOR_OPEN = GPIO_PIN_SET,
} ContactorState_t;

void setNegContactor(ContactorState_t state)
{
    DEBUG_PRINT("%s negative contactor\n", state==CONTACTOR_CLOSED?"Closing":"Opening");
    if (state==CONTACTOR_CLOSED) CONT_NEG_CLOSE;
    else if (state == CONTACTOR_OPEN) CONT_NEG_OPEN;
}

void setPosContactor(ContactorState_t state)
{
    DEBUG_PRINT("%s positive contactor\n", state==CONTACTOR_CLOSED?"Closing":"Opening");

    if (state==CONTACTOR_CLOSED) CONT_POS_CLOSE;
    else if (state == CONTACTOR_OPEN) CONT_POS_OPEN;
}

void setPrechargeContactor(ContactorState_t state)
{
    DEBUG_PRINT("%s precharge contactor\n", state==CONTACTOR_CLOSED?"Closing":"Opening");

    if (state==CONTACTOR_CLOSED) PCDC_PC;
    else if (state == CONTACTOR_OPEN) PCDC_DC;
}

void openAllContactors()
{
    setPosContactor(CONTACTOR_OPEN);
    setNegContactor(CONTACTOR_OPEN);
    setPrechargeContactor(CONTACTOR_OPEN);
}

typedef enum Precharge_Discharge_Return_t {
    PCDC_DONE,
    PCDC_STOPPED,
    PCDC_ERROR
} Precharge_Discharge_Return_t;


#define WAIT_FOR_NEXT_MEASURE_OR_STOP(MeasurePeriod, NotificationOut) \
    do { \
        NotificationOut = 0; /* Set to zero, in case no notifaction received */ \
        xTaskNotifyWait( 0x00,       /* Don't clear any notification bits on entry. */ \
                         UINT32_MAX, /* Reset the notification value to 0 on exit. */ \
                         &NotificationOut, /* Notified value passed out */ \
                         pdMS_TO_TICKS(MeasurePeriod)); /* Timeout */ \
    } while(0)


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
Precharge_Discharge_Return_t precharge()
{
    float VBatt, VBus, IBus;
    uint32_t dbwTaskNotifications;

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
    }

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
        return PCDC_ERROR;
    }

    DEBUG_PRINT("PC Step 1\n");
    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    if (VBus > packVoltage * PRECHARGE_STEP_1_VBUS_MAX_PERCENT_VPACK)
    {
        ERROR_PRINT("ERROR: VBUS %f > %f\n", VBus,
                    packVoltage * PRECHARGE_STEP_1_VBUS_MAX_PERCENT_VPACK);
        return PCDC_ERROR;
    }
    ERROR_PRINT("INFO: VBatt %f\n", VBatt);
    if (VBatt > packVoltage * PRECHARGE_STEP_1_VBATT_MAX_PERCENT_VPACK) {
        ERROR_PRINT("ERROR: VBatt %f > %f\n", VBatt,
                    packVoltage * PRECHARGE_STEP_1_VBUS_MAX_PERCENT_VPACK);
        return PCDC_ERROR;
    }
    if (!HITL_Precharge_Mode) {
        // This check will fail on the HITL
        if (VBatt < packVoltage * PRECHARGE_STEP_1_VBATT_MIN_PERCENT_VPACK) {
            ERROR_PRINT("ERROR: VBatt %f > %f\n", VBatt,
                        packVoltage * PRECHARGE_STEP_1_VBATT_MIN_PERCENT_VPACK);
            return PCDC_ERROR;
        }
    }
    ERROR_PRINT("INFO: IBus %f\n", IBus);
    if (IBus > PRECHARGE_STEP_1_CURRENT_MAX) {
        ERROR_PRINT("ERROR: VBatt %f > %f\n", IBus, PRECHARGE_STEP_1_CURRENT_MAX);
        return PCDC_ERROR;
    }

    /*
     * Step 2:
     * IShunt == 0
     * VBUS = 0
     * VBATT = VPACK
     * Close Precharge contactor
     */
    setPrechargeContactor(CONTACTOR_CLOSED);
    setPosContactor(CONTACTOR_OPEN);
    setNegContactor(CONTACTOR_OPEN);

    uint32_t startTickCount = xTaskGetTickCount();
    do {
        // Delay for contactors to close and measurements to stabilize
        WAIT_FOR_NEXT_MEASURE_OR_STOP(PRECHARGE_STEP_2_WAIT_TIME_MS,
                                      dbwTaskNotifications);
        if (dbwTaskNotifications & (1<<STOP_NOTIFICATION)) {
            DEBUG_PRINT("Precharge Stopped\n");
            return PCDC_STOPPED;
        }

        if (updateMeasurements(&VBus, &VBatt, &IBus) != HAL_OK) {
            return PCDC_ERROR;
        }

        if (xTaskGetTickCount() - startTickCount > PRECHARGE_STEP_2_TIMEOUT) {
            ERROR_PRINT("Precharge step 2 timed out waiting for IBus to zero\n");
            ERROR_PRINT("INFO: VBUS %f\n", VBus);
            ERROR_PRINT("INFO: VBatt %f\n", VBatt);
            ERROR_PRINT("INFO: IBus %f\n", IBus);
            return PCDC_ERROR;
        }
    } while (IBus > PRECHARGE_STEP_2_CURRENT_MAX);

    DEBUG_PRINT("PC Step 2\n");
    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    if (VBus > packVoltage * PRECHARGE_STEP_2_VBUS_MAX_PERCENT_VPACK)
    {
        ERROR_PRINT("ERROR: VBUS %f > %f\n", VBus,
                    packVoltage * PRECHARGE_STEP_2_VBUS_MAX_PERCENT_VPACK);
        return PCDC_ERROR;
    }
    ERROR_PRINT("INFO: VBatt %f\n", VBatt);
    if (VBatt < packVoltage*PRECHARGE_STEP_2_PERCENT_VBATT_MIN) {
        ERROR_PRINT("ERROR: VBatt %f < %f\n", VBatt, packVoltage*PRECHARGE_STEP_2_PERCENT_VBATT_MIN);
        return PCDC_ERROR;
    }
    ERROR_PRINT("INFO: IBus %f\n", IBus);

    // Store the pack voltage to use in step 4
    // Check to make sure it is close to what AMS boards think is pack voltage
    if (fabs(VBatt - packVoltage) > PACK_VOLTAGE_TOLERANCE_PERCENT*packVoltage) {
        ERROR_PRINT("VBatt measurement %f different than packVoltage %f\n",
                    VBatt, packVoltage);
        return PCDC_ERROR;
    }
    packVoltage = VBatt;


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

    DEBUG_PRINT("PC Step 2\n");
    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    if (VBus > packVoltage * PRECHARGE_STEP_3_VBUS_MAX_PERCENT_VPACK)
    {
        ERROR_PRINT("ERROR: VBUS %f > %f\n", VBus,
                    packVoltage * PRECHARGE_STEP_3_VBUS_MAX_PERCENT_VPACK);
        return PCDC_ERROR;
    }
    ERROR_PRINT("INFO: VBatt %f\n", VBatt);
    if (VBatt > packVoltage * PRECHARGE_STEP_3_VBATT_MAX_PERCENT_VPACK) {
        ERROR_PRINT("ERROR: VBatt %f > %f\n", VBatt,
                    packVoltage * PRECHARGE_STEP_3_VBUS_MAX_PERCENT_VPACK);
        return PCDC_ERROR;
    }
    if (!HITL_Precharge_Mode) {
        // This check will fail on the HITL
        if (VBatt < packVoltage * PRECHARGE_STEP_3_VBATT_MIN_PERCENT_VPACK) {
            ERROR_PRINT("ERROR: VBatt %f > %f\n", VBatt,
                        packVoltage * PRECHARGE_STEP_3_VBATT_MIN_PERCENT_VPACK);
            return PCDC_ERROR;
        }
    }
    ERROR_PRINT("INFO: IBus %f\n", IBus);
    if (IBus > PRECHARGE_STEP_3_CURRENT_MAX) {
        ERROR_PRINT("ERROR: VBatt %f > %f\n", IBus, PRECHARGE_STEP_3_CURRENT_MAX);
        return PCDC_ERROR;
    }

    /*
     * Step 4:
     * IShunt >= 1
     * VBUS = 0
     * VBATT = VPACK
     * precharge closed, neg contactor closed, pos contactor open
     */

    setPrechargeContactor(CONTACTOR_CLOSED);
    setNegContactor(CONTACTOR_CLOSED);
    setPosContactor(CONTACTOR_OPEN);

    float maxIBus = 0;
    startTickCount = xTaskGetTickCount();
    do {
        if (updateMeasurements(&VBus, &VBatt, &IBus) != HAL_OK) {
            return PCDC_ERROR;
        }
        /*ERROR_PRINT("INFO: VBUS %f\n", VBus);*/
        /*ERROR_PRINT("INFO: VBatt %f\n", VBatt);*/
        /*ERROR_PRINT("INFO: IBus %f\n", IBus);*/
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
            return PCDC_ERROR;
        }
    } while (VBus < (packVoltage*PRECHARGE_STEP_4_COMPLETE_PERCENT_VPACK));

    uint32_t prechargeTime = xTaskGetTickCount() - startTickCount;
    DEBUG_PRINT("Precharge took %lu ticks\n", prechargeTime);
    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    ERROR_PRINT("INFO: VBatt %f\n", VBatt);
    ERROR_PRINT("INFO: IBus %f\n", IBus);

    float minPrechargeCurrent = (packVoltage) / PRECHARGE_RESISTOR_OHMS;
    minPrechargeCurrent *= MIN_PRECHARGE_PERCENT_IDEAL_CURRENT;
    DEBUG_PRINT("Info: Max IBus: %f, needed %f\n", maxIBus, minPrechargeCurrent);
    if (maxIBus < minPrechargeCurrent) {
        ERROR_PRINT("Didn't detect precharge current!\n");
        ERROR_PRINT("Max IBus: %f, needed %f\n", maxIBus, minPrechargeCurrent);
        return PCDC_ERROR;
    }

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
    do {
        if (updateMeasurements(&VBus, &VBatt, &IBus) != HAL_OK) {
            return PCDC_ERROR;
        }
        /*ERROR_PRINT("INFO: VBUS %f\n", VBus);*/
        /*ERROR_PRINT("INFO: VBatt %f\n", VBatt);*/
        /*ERROR_PRINT("INFO: IBus %f\n", IBus);*/
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
            return PCDC_ERROR;
        }
    } while (VBus < (packVoltage*PRECHARGE_STEP_5_COMPLETE_PERCENT_VPACK));

    float minIBusSpike = (packVoltage - step4EndVBus) / PRECHARGE_RESISTOR_OHMS;
    minIBusSpike *= PRECHARGE_STEP_5_PERCENT_IDEAL_CURRENT_REQUIRED;

    DEBUG_PRINT("Info: Max IBus: %f, needed %f\n", maxIBus, minIBusSpike);
    if (maxIBus < minIBusSpike) {
        ERROR_PRINT("IBus %f, required spike %f\n", maxIBus, minIBusSpike);
        return PCDC_ERROR;
    }

    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    ERROR_PRINT("INFO: VBatt %f\n", VBatt);
    ERROR_PRINT("INFO: IBus %f\n", IBus);

    DEBUG_PRINT("Finished Precharge\n");
    return PCDC_DONE;
}

Precharge_Discharge_Return_t discharge()
{
    sendDTC_WARNING_CONTACTOR_OPEN_IMPENDING();
    DEBUG_PRINT("Discharge start, waiting for zero current\n");
    uint32_t startTickVal = xTaskGetTickCount();
    float IBus, VBus, VBatt;
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
        vTaskDelay(1);
    } while (IBus > ZERO_CURRENT_MAX_AMPS);
    DEBUG_PRINT("Opening contactors\n");
    openAllContactors();

    uint32_t startTickCount = xTaskGetTickCount();
    do {
        if (updateMeasurements(&VBus, &VBatt, &IBus) != HAL_OK) {
            return PCDC_ERROR;
        }
        if (xTaskGetTickCount() - startTickCount > PRECHARGE_STEP_4_TIMEOUT) {
            ERROR_PRINT("Discharge timed out\n");
            ERROR_PRINT("INFO: VBUS %f\n", VBus);
            ERROR_PRINT("INFO: VBatt %f\n", VBatt);
            ERROR_PRINT("INFO: IBus %f\n", IBus);
            return PCDC_ERROR;
        }

        vTaskDelay(pdMS_TO_TICKS(DISCHARGE_MEASURE_PERIOD_MS));
    } while (VBus > DISCHARGE_DONE_BUS_VOLTAGE);

    ERROR_PRINT("INFO: VBUS %f\n", VBus);
    ERROR_PRINT("INFO: VBatt %f\n", VBatt);
    ERROR_PRINT("INFO: IBus %f\n", IBus);

    DEBUG_PRINT("Discharge done\n");

    return PCDC_DONE;
}

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

        } else if (dbwTaskNotifications & (1<<PRECHARGE_NOTIFICATION)) {
            DEBUG_PRINT("Starting precharge\n");
            rc = precharge();

            if (rc == PCDC_DONE) {
                DEBUG_PRINT("Precharge done\n");
                fsmSendEvent(&fsmHandle, EV_Precharge_Finished, portMAX_DELAY);
            } else if (rc == PCDC_ERROR) {
                DEBUG_PRINT("Precharge Error\n");
                discharge();
                fsmSendEvent(&fsmHandle, EV_PrechargeDischarge_Fail, portMAX_DELAY);
            } else {
                DEBUG_PRINT("Precharge Stopped\n");
                discharge();
                // TODO: Is there anything to do if the precharge gets stopped?
            }

        } else if (dbwTaskNotifications & (1<<DISCHARGE_NOTIFICATION)) {
            if (discharge() != PCDC_DONE) {
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

