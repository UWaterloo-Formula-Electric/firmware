#ifndef __STATE_MACHINE_H
#define __STATE_MACHINE_H
#include "bsp.h"

// To use this state machine framework, implement an enum of states, an enum of
// events, and a transition table in your code

typedef uint32_t Event_t;

typedef struct {
    uint32_t st;         // The state where this transition occurs from
    Event_t ev;         // The Event that causes this transition

    uint32_t (*fn)(uint32_t); // Function called when event occurs and fsm is in the specified state
                              // The function should take one int, which is the event
                              // that called the function to be called, and return the
                              // new state
} Transition_t;

typedef struct {
    uint32_t maxStateNum;
    uint32_t maxEventNum;
    uint32_t sizeofEventEnumType;
    uint32_t transitionTableLength;
    uint32_t ST_ANY;
    uint32_t EV_ANY;
    Transition_t *transitions;
} FSM_Init_Struct;

// Get the size of a transition table, only work when passing in the original
// transition table array, not a pointer to it
#define TRANS_COUNT(transition_table) \
    (sizeof(transition_table)/sizeof(*transition_table))

HAL_StatusTypeDef fsmInit(uint32_t startingState, FSM_Init_Struct *init);
HAL_StatusTypeDef fsmProcessEvent(uint32_t event);
uint32_t fsmGetState();
#endif // __STATE_MACHINE_H
