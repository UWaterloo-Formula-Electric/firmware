#include "prechargeDischarge.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "freertos.h"
#include "task.h"
#include "math.h"


#ifdef MOCK_MEASUREMENTS
float I_Shunt;
float VBatt;
float VBus;
#endif

HAL_StatusTypeDef pcdcInit()
{
    DEBUG_PRINT("PCDC Init\n");
    return HAL_OK;
}

float getIshunt(void)
{
#ifdef MOCK_MEASUREMENTS
    return I_Shunt;
#else
#endif
}

float getVBatt(void)
{
#ifdef MOCK_MEASUREMENTS
    return VBatt;
#else
#endif
}

float getVBus(void)
{
#ifdef MOCK_MEASUREMENTS
    return VBus;
#else
#endif
}

typedef enum Precharge_Discharge_Return_t {
    PCDC_DONE,
    PCDC_STOPPED,
    PCDC_ERROR
} Precharge_Discharge_Return_t;


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
                              uint32_t waitTimeMs, uint32_t currentMeasurePeriodMs)
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
        return PCDC_ERROR;
    }
    if (!MEASUREMENT_IN_RANGE(VBus,
                              vBusVal,
                              vBusRange))
    {
        ERROR_PRINT("VBus out of range for precharge: %f\n", VBus);
        return PCDC_ERROR;
    }

    return PCDC_DONE;
}

Precharge_Discharge_Return_t precharge()
{
    Precharge_Discharge_Return_t rc;

    /*
     * Step 1:
     * IShunt == 0
     * VBUS = 0
     * VBATT = VPACK / 2
     */

#ifdef MOCK_MEASUREMENTS
#ifdef PC_STEP_1_Succeed
    I_Shunt = 0;
    VBus = 0;
    VBatt = VPACK/2;
#endif
#endif

    DEBUG_PRINT("Precharge step 1 start\n");
    rc = measureCurrentAndVoltages(PRECHARGE_STEP_1_CURRENT_VAL, PRECHARGE_STEP_1_CURRENT_RANGE,
                              PRECHARGE_STEP_1_VBUS_VAL, PRECHARGE_STEP_1_VBUS_RANGE,
                              PRECHARGE_STEP_1_VBATT_VAL, PRECHARGE_STEP_1_VBATT_RANGE,
                              PRECHARGE_STEP_1_WAIT_TIME_MS,
                              PRECHARGE_STEP_1_CURRENT_MEASURE_PERIOD_MS);

    if (rc == PCDC_ERROR) {
        // TODO: Handle step 1 error
        return PCDC_ERROR;
    } else if (rc == PCDC_STOPPED) {
        // TODO: Handle step 1 stop
        return PCDC_STOPPED;
    }

    /*
     * Step 2:
     * IShunt == 0
     * VBUS = 0
     * VBATT = VPACK
     */

#ifdef MOCK_MEASUREMENTS
#ifdef PC_STEP_2_Succeed
    I_Shunt = 0;
    VBus = 0;
    VBatt = VPACK;
#endif
#endif

    DEBUG_PRINT("Precharge step 2 start\n");
    rc = measureCurrentAndVoltages(PRECHARGE_STEP_2_CURRENT_VAL, PRECHARGE_STEP_2_CURRENT_RANGE,
                              PRECHARGE_STEP_2_VBUS_VAL, PRECHARGE_STEP_2_VBUS_RANGE,
                              PRECHARGE_STEP_2_VBATT_VAL, PRECHARGE_STEP_2_VBATT_RANGE,
                              PRECHARGE_STEP_2_WAIT_TIME_MS,
                              PRECHARGE_STEP_2_CURRENT_MEASURE_PERIOD_MS);

    if (rc == PCDC_ERROR) {
        // TODO: Handle step 2 error
        return PCDC_ERROR;
    } else if (rc == PCDC_STOPPED) {
        // TODO: Handle step 2 stop
        return PCDC_STOPPED;
    }

    /*
     * Step 3:
     * IShunt == 0
     * VBUS = 0
     * VBATT = VPACK / 2
     */

#ifdef MOCK_MEASUREMENTS
#ifdef PC_STEP_3_Succeed
    I_Shunt = 0;
    VBus = 0;
    VBatt = VPACK/2;
#endif
#endif

    DEBUG_PRINT("Precharge step 3 start\n");
    rc = measureCurrentAndVoltages(PRECHARGE_STEP_3_CURRENT_VAL, PRECHARGE_STEP_3_CURRENT_RANGE,
                              PRECHARGE_STEP_3_VBUS_VAL, PRECHARGE_STEP_3_VBUS_RANGE,
                              PRECHARGE_STEP_3_VBATT_VAL, PRECHARGE_STEP_3_VBATT_RANGE,
                              PRECHARGE_STEP_3_WAIT_TIME_MS,
                              PRECHARGE_STEP_3_CURRENT_MEASURE_PERIOD_MS);

    if (rc == PCDC_ERROR) {
        // TODO: Handle step 3 error
        return PCDC_ERROR;
    } else if (rc == PCDC_STOPPED) {
        // TODO: Handle step 3 stop
        return PCDC_STOPPED;
    }

    /*
     * Step 4:
     * IShunt >= 1
     * VBUS = 0
     * VBATT = VPACK
     */

#ifdef MOCK_MEASUREMENTS
#ifdef PC_STEP_4_Succeed
    I_Shunt = 10;
    VBus = 0;
    VBatt = VPACK;
#endif
#endif

    DEBUG_PRINT("Precharge step 4 start\n");
    rc = measureCurrentAndVoltages(PRECHARGE_STEP_4_CURRENT_VAL, PRECHARGE_STEP_4_CURRENT_RANGE,
                              PRECHARGE_STEP_4_VBUS_VAL, PRECHARGE_STEP_4_VBUS_RANGE,
                              PRECHARGE_STEP_4_VBATT_VAL, PRECHARGE_STEP_4_VBATT_RANGE,
                              PRECHARGE_STEP_4_WAIT_TIME_MS,
                              PRECHARGE_STEP_4_CURRENT_MEASURE_PERIOD_MS);

    if (rc == PCDC_ERROR) {
        // TODO: Handle step 4 error
        return PCDC_ERROR;
    } else if (rc == PCDC_STOPPED) {
        // TODO: Handle step 4 stop
        return PCDC_STOPPED;
    }

    /*
     * Step 5:
     * IShunt >= 0
     * VBUS = VPACK
     * VBATT = VPACK
     */

#ifdef MOCK_MEASUREMENTS
#ifdef PC_STEP_5_Succeed
    I_Shunt = 3;
    VBus = VPACK;
    VBatt = VPACK;
#endif
#endif

    DEBUG_PRINT("Precharge step 5 start\n");
    rc = measureCurrentAndVoltages(PRECHARGE_STEP_5_CURRENT_VAL, PRECHARGE_STEP_5_CURRENT_RANGE,
                              PRECHARGE_STEP_5_VBUS_VAL, PRECHARGE_STEP_5_VBUS_RANGE,
                              PRECHARGE_STEP_5_VBATT_VAL, PRECHARGE_STEP_5_VBATT_RANGE,
                              PRECHARGE_STEP_5_WAIT_TIME_MS,
                              PRECHARGE_STEP_5_CURRENT_MEASURE_PERIOD_MS);

    if (rc == PCDC_ERROR) {
        // TODO: Handle step 5 error
        return PCDC_ERROR;
    } else if (rc == PCDC_STOPPED) {
        // TODO: Handle step 5 stop
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

            // TODO: Implement discharge
            DEBUG_PRINT("Discharge start\n");
            vTaskDelay(1000);
            DEBUG_PRINT("Discharge done\n");
            fsmSendEvent(&fsmHandle, EV_Discharge_Finished, portMAX_DELAY);

        } else {
            ERROR_PRINT("Unkown notification received 0x%lX\n", dbwTaskNotifications);
        }
    }
}

