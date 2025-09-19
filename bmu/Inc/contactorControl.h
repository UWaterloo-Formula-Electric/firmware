#ifndef CONTACTOR_CONTROL_H
#define CONTACTOR_CONTROL_H

#include "controlStateMachine.h"

#define CONTACTOR_SENSE_PERIOD 2000

typedef enum {
    CONT_NEG_SENSE_INDEX = 0,
    CONT_POS_SENSE_INDEX,
    THERMISTOR_INDEX,
    NUM_CONT_THERMISTOR_INDEX,
}contactorThermistorIndex_e;

/**
 * Enum to translate between contactor open/closed and GPIO pin state
 */
typedef enum ContactorState_t {
    CONTACTOR_CLOSED = GPIO_PIN_RESET,
    CONTACTOR_OPEN = GPIO_PIN_SET,
} ContactorState_t;

void setNegContactor(ContactorState_t state);
void setPosContactor(ContactorState_t state);
void setPrechargeContactor(ContactorState_t state);
void openAllContactors();

#endif
