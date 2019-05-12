#ifndef PRECHARGEDISCHARGE_H

#define PRECHARGEDISCHARGE_H

#include "main.h"
#include "freertos.h"

typedef enum PCDC_Notifications_t {
    PRECHARGE_NOTIFICATION,
    DISCHARGE_NOTIFICATION,
    STOP_NOTIFICATION,
} PCDC_Notifications_t;

/*
 * Discharge constants
 */

#define DISCHARGE_MEASURE_PERIOD_MS 100
#define DISCHARGE_DONE_BUS_VOLTAGE (0.2F)

#define ZERO_CURRENT_MAX_AMPS (0.01)
#define CONTACTOR_OPEN_ZERO_CURRENT_TIMEOUT_MS 50

/*
 * Precharge constants
 */

#define PRECHARGE_RESISTOR_OHMS (10000)

// Timeouts
#define PRECHARGE_STEP_1_WAIT_TIME_MS 2000

#define PRECHARGE_STEP_2_WAIT_TIME_MS 2000

#define PRECHARGE_STEP_3_WAIT_TIME_MS 2000

#define PRECHARGE_STEP_4_CURRENT_MEASURE_PERIOD_MS (5)
#define PRECHARGE_STEP_4_TIMEOUT 20000

#define PRECHARGE_STEP_5_CURRENT_MEASURE_PERIOD_MS (1)
#define PRECHARGE_STEP_5_TIMEOUT 1000


/* only used for testing in cli */
HAL_StatusTypeDef getIBus(float *IBus);
HAL_StatusTypeDef getVBatt(float *VBatt);
HAL_StatusTypeDef getVBus(float * VBus);

#endif /* end of include guard: PRECHARGEDISCHARGE_H */
