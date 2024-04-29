#ifndef COOLING_H

#define COOLING_H

#define COOLING_TASK_PERIOD_MS 10000 // 5 seconds
#define COOLING_INCREMENT_MS 100 // 100 ms
#define COOLING_AMBIENT_TEMP_C 15.0f // Todo - figure out how to acc get ambient
#define COOLING_MOTOR_MAX_TEMP_C 90.0f
#define COOLING_INV_MAX_TEMP_C 90.0f
#define MC_MESSAGE_LOST_THRESHOLD_MS 200 

typedef enum Cooling_States_t {
    COOL_STATE_OFF,
    COOL_STATE_WAIT,
    COOL_STATE_ON,
    COOL_STATE_HV_CRITICAL,
    COOL_STATE_LV_Cuttoff,
    COOL_STATE_ANY, // Must be the last state
} Cooling_States_t;

void coolingOff(void);
void coolingOn(void);

#endif /* end of include guard: COOLING_H */
