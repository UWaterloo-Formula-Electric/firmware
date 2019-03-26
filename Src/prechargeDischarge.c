#include "prechargeDischarge.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "freertos.h"
#include "task.h"
#include "math.h"
#include "BMU_DTC.h"
#include "batteries.h"

#define ZERO_CURRENT_MAX_AMPS (0.5)
#define CONTACTOR_OPEN_ZERO_CURRENT_TIMEOUT_MS 50

HAL_StatusTypeDef pcdcInit()
{
    DEBUG_PRINT("PCDC Init\n");
    return HAL_OK;
}

float getIshunt(void)
{
    float Ishunt;

    if (xQueuePeek(IBusQueue, &Ishunt, 0) != pdTRUE) {
        ERROR_PRINT("Failed to receive Ishunt current from queue\n");
        return -1;
    }

    return Ishunt;
}

float getVBatt(void)
{
    float VBatt;

    if (xQueuePeek(VBattQueue, &VBatt, 0) != pdTRUE) {
        ERROR_PRINT("Failed to receive VBatt voltage from queue\n");
        return -1;
    }

    return VBatt;
}

float getVBus(void)
{
    float VBus;

    if (xQueuePeek(VBusQueue, &VBus, 0) != pdTRUE) {
        ERROR_PRINT("Failed to receive VBus voltage from queue\n");
        return -1;
    }

    return VBus;
}

typedef enum ContactorState_t {
    CONTACTOR_CLOSED = GPIO_PIN_RESET,
    CONTACTOR_OPEN = GPIO_PIN_SET,
} ContactorState_t;

void setNegContactor(ContactorState_t state)
{
    DEBUG_PRINT("%s negative contactor\n", state==CONTACTOR_CLOSED?"Closing":"Opening");

#if IS_BOARD_F7
    HAL_GPIO_WritePin(CONT_NEG_GPIO_Port, CONT_NEG_Pin, state);
#endif
}

void setPosContactor(ContactorState_t state)
{
    DEBUG_PRINT("%s positive contactor\n", state==CONTACTOR_CLOSED?"Closing":"Opening");

#if IS_BOARD_F7
    HAL_GPIO_WritePin(CONT_POS_GPIO_Port, CONT_POS_Pin, state);
#endif
}

void setPrechargeContactor(ContactorState_t state)
{
    DEBUG_PRINT("%s precharge contactor\n", state==CONTACTOR_CLOSED?"Closing":"Opening");

#if IS_BOARD_F7
    HAL_GPIO_WritePin(CONT_PRE_GPIO_Port, CONT_PRE_Pin, state);
#endif
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

typedef enum Precharge_Discharge_FailureType {
    PCDC_IBUS_FAIL,
    PCDC_VBATT_FAIL,
    PCDC_VBUS_FAIL,
    PCDC_ALL_OK
} Precharge_Discharge_FailureType;


#define MEASUREMENT_IN_RANGE(val, target, range) (fabsf((val) - (target)) <= range) 

#define WHILE_NOT_TIMEOUT(timeout) \
    uint32_t startCount = xTaskGetTickCount(); \
    while (xTaskGetTickCount() - startCount <= pdMS_TO_TICKS(timeout))

#define WAIT_FOR_NEXT_MEASURE_OR_STOP(MeasurePeriod, NotificationOut) \
    do { \
        NotificationOut = 0; /* Set to zero, in case no notifaction received */ \
        xTaskNotifyWait( 0x00,       /* Don't clear any notification bits on entry. */ \
                         UINT32_MAX, /* Reset the notification value to 0 on exit. */ \
                         &NotificationOut, /* Notified value passed out */ \
                         pdMS_TO_TICKS(MeasurePeriod)); /* Timeout */ \
    } while(0)


Precharge_Discharge_Return_t 
    measureCurrentAndVoltages(float currentVal, float currentRange, float vBusVal, 
                              float vBusRange, float vBattVal, float vBattRange,
                              uint32_t waitTimeMs, uint32_t currentMeasurePeriodMs,
                              Precharge_Discharge_FailureType *failure)
{
    uint32_t dbwTaskNotifications;

    WHILE_NOT_TIMEOUT(waitTimeMs) {
        WAIT_FOR_NEXT_MEASURE_OR_STOP(currentMeasurePeriodMs,
                                      dbwTaskNotifications);

        if (!(dbwTaskNotifications & (1<<STOP_NOTIFICATION))) {
            float IShunt = getIshunt();
            if (!MEASUREMENT_IN_RANGE(IShunt,
                                     currentVal,
                                     currentRange))
            {
                ERROR_PRINT("IShunt out of range for precharge: %f\n", IShunt); 
                (*failure) = PCDC_IBUS_FAIL;
                return PCDC_ERROR;
            }

        } else {
            // handle stop
            DEBUG_PRINT("Receive stop during precharge\n");
            return PCDC_STOPPED;
        }
    }

    float VBatt = getVBatt();
    float VBus = getVBus();

    if (!MEASUREMENT_IN_RANGE(VBatt,
                              vBattVal,
                              vBattRange))
    {
        ERROR_PRINT("VBatt out of range for precharge: %f\n", VBatt);
        (*failure) = PCDC_VBATT_FAIL;
        return PCDC_ERROR;
    }
    if (!MEASUREMENT_IN_RANGE(VBus,
                              vBusVal,
                              vBusRange))
    {
        ERROR_PRINT("VBus out of range for precharge: %f\n", VBus);
        (*failure) = PCDC_VBUS_FAIL;
        return PCDC_ERROR;
    }

    (*failure) = PCDC_ALL_OK;
    return PCDC_DONE;
}

Precharge_Discharge_Return_t precharge()
{
    Precharge_Discharge_Return_t rc;
    Precharge_Discharge_FailureType failure;

    /*
     * Step 1:
     * IShunt == 0
     * VBUS = 0
     * VBATT = VPACK / 2
     */

#ifdef PC_STEP_1_Succeed
    IBus = 0;
    VBus = 0;
    VBatt = VPACK/2;
    vTaskDelay(500); // Delay to allow battery task to publish these values
#endif

    setPrechargeContactor(CONTACTOR_OPEN);
    setPosContactor(CONTACTOR_OPEN);
    setNegContactor(CONTACTOR_OPEN);

    DEBUG_PRINT("Precharge step 1 start\n");
    rc = measureCurrentAndVoltages(PRECHARGE_STEP_1_CURRENT_VAL, PRECHARGE_STEP_1_CURRENT_RANGE,
                              PRECHARGE_STEP_1_VBUS_VAL, PRECHARGE_STEP_1_VBUS_RANGE,
                              PRECHARGE_STEP_1_VBATT_VAL, PRECHARGE_STEP_1_VBATT_RANGE,
                              PRECHARGE_STEP_1_WAIT_TIME_MS,
                              PRECHARGE_STEP_1_CURRENT_MEASURE_PERIOD_MS,
                              &failure);

    if (rc == PCDC_ERROR) {
        openAllContactors();
        sendDTC_FATAL_PrechargeFailed(1);
        return PCDC_ERROR;
    } else if (rc == PCDC_STOPPED) {
        openAllContactors();
        return PCDC_STOPPED;
    }

    /*
     * Step 2:
     * IShunt == 0
     * VBUS = 0
     * VBATT = VPACK
     * Close Precharge contactor
     */

#ifdef PC_STEP_2_Succeed
    IBus = 0;
    VBus = 0;
    VBatt = VPACK;
#endif

    setPrechargeContactor(CONTACTOR_CLOSED);
    setPosContactor(CONTACTOR_OPEN);
    setNegContactor(CONTACTOR_OPEN);

    DEBUG_PRINT("Precharge step 2 start\n");
    rc = measureCurrentAndVoltages(PRECHARGE_STEP_2_CURRENT_VAL, PRECHARGE_STEP_2_CURRENT_RANGE,
                              PRECHARGE_STEP_2_VBUS_VAL, PRECHARGE_STEP_2_VBUS_RANGE,
                              PRECHARGE_STEP_2_VBATT_VAL, PRECHARGE_STEP_2_VBATT_RANGE,
                              PRECHARGE_STEP_2_WAIT_TIME_MS,
                              PRECHARGE_STEP_2_CURRENT_MEASURE_PERIOD_MS,
                              &failure);

    if (rc == PCDC_ERROR) {
        openAllContactors();
        sendDTC_FATAL_PrechargeFailed(2);
        return PCDC_ERROR;
    } else if (rc == PCDC_STOPPED) {
        openAllContactors();
        return PCDC_STOPPED;
    }

    /*
     * Step 3:
     * IShunt == 0
     * VBUS = 0
     * VBATT = VPACK / 2
     * Precharge open, neg contactor close, pos contactor open
     */

    setPrechargeContactor(CONTACTOR_OPEN);
    setNegContactor(CONTACTOR_CLOSED);
    setPosContactor(CONTACTOR_OPEN);

#ifdef PC_STEP_3_Succeed
    IBus = 0;
    VBus = 0;
    VBatt = VPACK/2;
#endif

    DEBUG_PRINT("Precharge step 3 start\n");
    rc = measureCurrentAndVoltages(PRECHARGE_STEP_3_CURRENT_VAL, PRECHARGE_STEP_3_CURRENT_RANGE,
                              PRECHARGE_STEP_3_VBUS_VAL, PRECHARGE_STEP_3_VBUS_RANGE,
                              PRECHARGE_STEP_3_VBATT_VAL, PRECHARGE_STEP_3_VBATT_RANGE,
                              PRECHARGE_STEP_3_WAIT_TIME_MS,
                              PRECHARGE_STEP_3_CURRENT_MEASURE_PERIOD_MS,
                              &failure);

    if (rc == PCDC_ERROR) {
        openAllContactors();
        sendDTC_FATAL_PrechargeFailed(3);
        return PCDC_ERROR;
    } else if (rc == PCDC_STOPPED) {
        openAllContactors();
        return PCDC_STOPPED;
    }

    /*
     * Step 4:
     * IShunt >= 1
     * VBUS = 0
     * VBATT = VPACK
     * precharge closed, neg contactor closed, pos contactor open
     */

#ifdef PC_STEP_4_Succeed
    IBus = 10;
    VBus = 0;
    VBatt = VPACK;
#endif

    setPrechargeContactor(CONTACTOR_CLOSED);
    setNegContactor(CONTACTOR_CLOSED);
    setPosContactor(CONTACTOR_OPEN);

    DEBUG_PRINT("Precharge step 4 start\n");
    rc = measureCurrentAndVoltages(PRECHARGE_STEP_4_CURRENT_VAL, PRECHARGE_STEP_4_CURRENT_RANGE,
                              PRECHARGE_STEP_4_VBUS_VAL, PRECHARGE_STEP_4_VBUS_RANGE,
                              PRECHARGE_STEP_4_VBATT_VAL, PRECHARGE_STEP_4_VBATT_RANGE,
                              PRECHARGE_STEP_4_WAIT_TIME_MS,
                              PRECHARGE_STEP_4_CURRENT_MEASURE_PERIOD_MS,
                              &failure);

    if (rc == PCDC_ERROR) {
        openAllContactors();
        sendDTC_FATAL_PrechargeFailed(4);
        return PCDC_ERROR;
    } else if (rc == PCDC_STOPPED) {
        openAllContactors();
        return PCDC_STOPPED;
    }

    /*
     * Step 5:
     * IShunt >= 0
     * VBUS = VPACK
     * VBATT = VPACK
     */

#ifdef PC_STEP_5_Succeed
    IBus = 3;
    VBus = VPACK;
    VBatt = VPACK;
#endif

    setPrechargeContactor(CONTACTOR_CLOSED);
    setNegContactor(CONTACTOR_CLOSED);
    setPosContactor(CONTACTOR_CLOSED);

    DEBUG_PRINT("Precharge step 5 start\n");
    rc = measureCurrentAndVoltages(PRECHARGE_STEP_5_CURRENT_VAL, PRECHARGE_STEP_5_CURRENT_RANGE,
                              PRECHARGE_STEP_5_VBUS_VAL, PRECHARGE_STEP_5_VBUS_RANGE,
                              PRECHARGE_STEP_5_VBATT_VAL, PRECHARGE_STEP_5_VBATT_RANGE,
                              PRECHARGE_STEP_5_WAIT_TIME_MS,
                              PRECHARGE_STEP_5_CURRENT_MEASURE_PERIOD_MS,
                              &failure);

    if (rc == PCDC_ERROR) {
        openAllContactors();
        sendDTC_FATAL_PrechargeFailed(5);
        return PCDC_ERROR;
    } else if (rc == PCDC_STOPPED) {
        openAllContactors();
        return PCDC_STOPPED;
    }

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
                fsmSendEvent(&fsmHandle, EV_PrechargeDischarge_Fail, portMAX_DELAY);
            } else {
                DEBUG_PRINT("Precharge Stopped\n");
                // TODO: Is there anything to do if the precharge gets stopped?
            }

        } else if (dbwTaskNotifications & (1<<DISCHARGE_NOTIFICATION)) {

            DEBUG_PRINT("Discharge start, waiting for zero current\n");
            sendDTC_WARNING_CONTACTOR_OPEN_IMPENDING();
            uint32_t startTickVal = xTaskGetTickCount();
            while (getIshunt() > ZERO_CURRENT_MAX_AMPS) {
                if (xTaskGetTickCount() - startTickVal >
                    pdMS_TO_TICKS(CONTACTOR_OPEN_ZERO_CURRENT_TIMEOUT_MS))
                {
                    ERROR_PRINT("Timed out waiting for zero current before opening contactors\n");
                    break;
                }
                vTaskDelay(1);
            }
            DEBUG_PRINT("Opening contactors\n");
            openAllContactors();
            DEBUG_PRINT("Discharge done\n");
            fsmSendEvent(&fsmHandle, EV_Discharge_Finished, portMAX_DELAY);

        } else {
            ERROR_PRINT("Unkown notification received 0x%lX\n", dbwTaskNotifications);
        }
    }
}

