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
 * Precharge constants
 */

// Timeouts
#define PRECHARGE_STEP_1_WAIT_TIME_MS 1000
#define PRECHARGE_STEP_1_CURRENT_MEASURE_PERIOD_MS ((PRECHARGE_STEP_1_WAIT_TIME_MS) / 10)

#define PRECHARGE_STEP_2_WAIT_TIME_MS 1000
#define PRECHARGE_STEP_2_CURRENT_MEASURE_PERIOD_MS ((PRECHARGE_STEP_2_WAIT_TIME_MS) / 10)

#define PRECHARGE_STEP_3_WAIT_TIME_MS 1000
#define PRECHARGE_STEP_3_CURRENT_MEASURE_PERIOD_MS ((PRECHARGE_STEP_3_WAIT_TIME_MS) / 10)

#define PRECHARGE_STEP_4_WAIT_TIME_MS 1000
#define PRECHARGE_STEP_4_CURRENT_MEASURE_PERIOD_MS ((PRECHARGE_STEP_4_WAIT_TIME_MS) / 10)

#define PRECHARGE_STEP_5_WAIT_TIME_MS 1000
#define PRECHARGE_STEP_5_CURRENT_MEASURE_PERIOD_MS ((PRECHARGE_STEP_5_WAIT_TIME_MS) / 10)

// Target Values and acceptable ranges (note: ranges are ± specified range val)

#define VPACK 300 // Volts

// Step 1
#define PRECHARGE_STEP_1_CURRENT_VAL 0 // Amps
#define PRECHARGE_STEP_1_CURRENT_RANGE 1 // Amps
#define PRECHARGE_STEP_1_VBUS_VAL 0 // Volts
#define PRECHARGE_STEP_1_VBUS_RANGE 5 // Volts
#define PRECHARGE_STEP_1_VBATT_VAL (VPACK/2) // Volts
#define PRECHARGE_STEP_1_VBATT_RANGE 50 // Volts

// Step 2
#define PRECHARGE_STEP_2_CURRENT_VAL 0 // Amps
#define PRECHARGE_STEP_2_CURRENT_RANGE 1 // Amps
#define PRECHARGE_STEP_2_VBUS_VAL 0 // Volts
#define PRECHARGE_STEP_2_VBUS_RANGE 5 // Volts
#define PRECHARGE_STEP_2_VBATT_VAL (VPACK) // Volts
#define PRECHARGE_STEP_2_VBATT_RANGE 50 // Volts

// Step 3
#define PRECHARGE_STEP_3_CURRENT_VAL 0 // Amps
#define PRECHARGE_STEP_3_CURRENT_RANGE 1 // Amps
#define PRECHARGE_STEP_3_VBUS_VAL 0 // Volts
#define PRECHARGE_STEP_3_VBUS_RANGE 5 // Volts
#define PRECHARGE_STEP_3_VBATT_VAL (VPACK/2) // Volts
#define PRECHARGE_STEP_3_VBATT_RANGE 50 // Volts

// Step 4
#define PRECHARGE_STEP_4_CURRENT_VAL 10 // Amps
#define PRECHARGE_STEP_4_CURRENT_RANGE 9 // Amps
#define PRECHARGE_STEP_4_VBUS_VAL 0 // Volts
#define PRECHARGE_STEP_4_VBUS_RANGE 5 // Volts
#define PRECHARGE_STEP_4_VBATT_VAL (VPACK) // Volts
#define PRECHARGE_STEP_4_VBATT_RANGE 50 // Volts

// Step 5
#define PRECHARGE_STEP_5_CURRENT_VAL 10 // Amps
#define PRECHARGE_STEP_5_CURRENT_RANGE 9 // Amps
#define PRECHARGE_STEP_5_VBUS_VAL (VPACK) // Volts
#define PRECHARGE_STEP_5_VBUS_RANGE 50 // Volts
#define PRECHARGE_STEP_5_VBATT_VAL (VPACK) // Volts
#define PRECHARGE_STEP_5_VBATT_RANGE 50 // Volts


/*
 * Mock testing defines/parameters/functions
 */

#define MOCK_MEASUREMENTS

// Uncomment these to have the code auto set I_Shunt, VBatt, and VBus to the
// right values before that step
#define PC_STEP_1_Succeed
#define PC_STEP_2_Succeed
#define PC_STEP_3_Succeed
#define PC_STEP_4_Succeed
#define PC_STEP_5_Succeed

#ifdef MOCK_MEASUREMENTS
extern float I_Shunt;
extern float VBatt;
extern float VBus;
#endif

float getIshunt(void);
float getVBatt(void);
float getVBus(void);

#endif /* end of include guard: PRECHARGEDISCHARGE_H */
