#ifndef __STATE_MACHINE_H
#define __STATE_MACHINE_H
#include "bsp.h"
#include "freertos.h"
#include "queue.h"

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
    uint32_t eventQueueLength;
} FSM_Init_Struct;

typedef struct {
    FSM_Init_Struct init;
    QueueHandle_t eventQueue;
    uint32_t state;
} FSM_Handle_Struct;


// Get the size of a transition table, only work when passing in the original
// transition table array, not a pointer to it
#define TRANS_COUNT(transition_table) \
    (sizeof(transition_table)/sizeof(*transition_table))

HAL_StatusTypeDef fsmInit(uint32_t startingState, FSM_Init_Struct *init,
                          FSM_Handle_Struct *handle);
void fsmTaskFunction(FSM_Handle_Struct *handle);
uint32_t fsmGetState(FSM_Handle_Struct *handle);
HAL_StatusTypeDef fsmSendEventUrgent(FSM_Handle_Struct *handle, uint32_t event, uint32_t timeout_ms);
HAL_StatusTypeDef fsmSendEvent(FSM_Handle_Struct *handle, uint32_t event, uint32_t timeout_ms);
HAL_StatusTypeDef fsmSendEventUrgentISR(FSM_Handle_Struct *handle, uint32_t event);
HAL_StatusTypeDef fsmSendEventISR(FSM_Handle_Struct *handle, uint32_t event);
#endif // __STATE_MACHINE_H
