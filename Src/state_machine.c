#include "state_machine.h"
#include "bsp.h"
#include "freertos.h"
#include "queue.h"
#include "string.h"
#include "debug.h"

#define EVENT_QUEUE_LENGTH 5

int fsmState;
FSM_Init_Struct fsmInfo;
QueueHandle_t fsmEventQueue;

HAL_StatusTypeDef fsmInit(uint32_t startingState, FSM_Init_Struct *init)
{
    fsmState = startingState;
    memcpy(&fsmInfo, init, sizeof(fsmInfo));

    fsmEventQueue = xQueueCreate(EVENT_QUEUE_LENGTH, fsmInfo.sizeofEventEnumType);

    if (fsmEventQueue == NULL)
    {
        ERROR_PRINT("Failed to create fsm event queue\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef fsmSendEvent(uint32_t event, uint32_t timeout_ms)
{
    if (xQueueSend(fsmEventQueue, &event, portTickType

HAL_StatusTypeDef fsmProcessEvent(uint32_t event)
{
    uint32_t newState;
    uint32_t i;
    Transition_t *trans = fsmInfo.transitions;

    DEBUG_PRINT("Processing event %lu\n", event);

    if (event > fsmInfo.maxEventNum) {
        ERROR_PRINT("FSM: Event out of range\n");
        return HAL_ERROR;
    }

    for (i = 0; i < fsmInfo.transitionTableLength; i++) {
        if ((fsmState == trans[i].st) || (fsmInfo.ST_ANY == trans[i].st)) {
            if ((event == trans[i].ev) || (fsmInfo.EV_ANY == trans[i].ev)) {
                newState = (trans[i].fn)(event);
                if (newState > fsmInfo.maxStateNum) {
                    ERROR_PRINT("FSM: New state out of range\n");
                    return HAL_ERROR;
                } else {
                    fsmState = newState;
                    break;
                }
            }
        }
    }

    if (i == fsmInfo.transitionTableLength) {
        DEBUG_PRINT("No matching transition found\n");
    }

    return HAL_OK;
}

uint32_t fsmGetState()
{
    return fsmState;
}
