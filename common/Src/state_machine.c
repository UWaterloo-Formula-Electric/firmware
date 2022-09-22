#include "state_machine.h"
#include "bsp.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "string.h"
#include "debug.h"
#include "watchdog.h"

#if BOARD_ID == ID_BMU
    #include "bmu_can.h"
#elif BOARD_ID  == ID_DCU
	#include "dcu_can.h"
#elif BOARD_ID  == ID_VCU_F7
	#include "vcu_F7_can.h"
#elif BOARD_ID  == ID_PDU
	#include "pdu_can.h"
#elif BOARD_ID  == ID_WSBFL
    #include "wsbfl_can.h"
#elif BOARD_ID  == ID_WSBFR
    #include "wsbfr_can.h"
#elif BOARD_ID  == ID_WSBRL
    #include "wsbrl_can.h"
#elif BOARD_ID  == ID_WSBRR
    #include "wsbrr_can.h"
#endif


HAL_StatusTypeDef fsmInit(uint32_t startingState, FSM_Init_Struct *init,
                          FSM_Handle_Struct *handle)
{
    memcpy(&(handle->init), init, sizeof(FSM_Init_Struct));

    if (startingState > init->maxStateNum) {
        ERROR_PRINT("Starting state out of range: %lu\n", startingState);
        return HAL_ERROR;
    }

    handle->state = startingState;

    if (handle->init.sizeofEventEnumType > sizeof(uint32_t)) {
        ERROR_PRINT("Event Enum type size exceeds chosen event size of event queue\n");
        return HAL_ERROR;
    }

    handle->eventQueue = xQueueCreate(handle->init.eventQueueLength, sizeof(uint32_t));

    if (handle->eventQueue == NULL)
    {
        ERROR_PRINT("Failed to create fsm event queue\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef fsmSendEventISR(FSM_Handle_Struct *handle, uint32_t event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (xQueueSendFromISR(handle->eventQueue, &event, &xHigherPriorityTaskWoken) != pdTRUE)
    {
        ERROR_PRINT_ISR("Failed to send event to queue\n");
        return HAL_ERROR;
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return HAL_OK;
}

HAL_StatusTypeDef fsmSendEventUrgentISR(FSM_Handle_Struct *handle, uint32_t event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (xQueueSendToFrontFromISR(handle->eventQueue, &event, &xHigherPriorityTaskWoken) != pdTRUE)
    {
        ERROR_PRINT_ISR("Failed to send event to queue\n");
        return HAL_ERROR;
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return HAL_OK;
}
HAL_StatusTypeDef fsmSendEvent(FSM_Handle_Struct *handle, uint32_t event, uint32_t timeout_ms)
{
    if (xQueueSend(handle->eventQueue, &event,
                   pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
    {
        ERROR_PRINT("Failed to send event to queue\n");
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef fsmSendEventUrgent(FSM_Handle_Struct *handle, uint32_t event, uint32_t timeout_ms)
{
    if (xQueueSendToFront(handle->eventQueue, &event,
                          pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
    {
        ERROR_PRINT("Failed to send event to queue\n");
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef fsmProcessEvent(FSM_Handle_Struct *handle, uint32_t event)
{
    uint32_t newState;
    uint32_t i;
    Transition_t *trans = handle->init.transitions;


    if (event == WATCHDOG_REQUEST_EVENT_NUM) {
        watchdogTaskCheckIn(handle->init.watchdogTaskId);
        return HAL_OK;
    }

    DEBUG_PRINT("Processing event %lu\n", event);

    if (event > handle->init.maxEventNum) {
        ERROR_PRINT("FSM: Event out of range\n");
        return HAL_ERROR;
    }

    for (i = 0; i < handle->init.transitionTableLength; i++) {
        if ((handle->state == trans[i].st) || (handle->init.ST_ANY == trans[i].st)) {
            if ((event == trans[i].ev) || (handle->init.EV_ANY == trans[i].ev)) {
                newState = (trans[i].fn)(event);
                if (newState > handle->init.maxStateNum) {
                    ERROR_PRINT("FSM: New state out of range\n");
                    return HAL_ERROR;
                } else {
                    handle->state = newState;
                    break;
                }
            }
        }
    }

    if (i == handle->init.transitionTableLength) {
        DEBUG_PRINT("No matching transition found\n");
    }

    return HAL_OK;
}

uint32_t fsmGetState(FSM_Handle_Struct *handle)
{
    return handle->state;
}

void fsmTaskFunction(FSM_Handle_Struct *handle)
{
    uint32_t event;

    while(1)
    {
        if (xQueueReceive(handle->eventQueue, &event, portMAX_DELAY) != pdTRUE)
        {
            ERROR_PRINT("Failed to receive from event queue, this shouldn't happen!\n");
            continue;
        }

        if (event != UINT32_MAX) {
            DEBUG_PRINT("Received event %lu\n", event);
        }
        
        
        StateMachineBoardID = BOARD_ID;
        StateMachineWatchdogID =  handle->init.watchdogTaskId;
        StateMachineEvent = event;
        StateMachineState = fsmGetState(handle);

        if (fsmProcessEvent(handle, event) != HAL_OK)
        {
            ERROR_PRINT("Failed to process event %lu\n", event);
        }

        StateMachineNewState = fsmGetState(handle);
        sendCAN_StateMachineEventProcessed(); 

    }
}

