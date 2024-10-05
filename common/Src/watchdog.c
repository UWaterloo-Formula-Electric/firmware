/*
 * In order for the watchdog to work, it needs to be enabled in CubeMX
 * The following parameters should be used, generating a WDG period of 20mS:
 * F7, input is 32kHz:
 * prescaler: 4
 * Window Value: 4095 (to disable windowing)
 * Counter reload value: 160
 * For F0, input is 40KHz, so use these parameters:
 * prescaler: 4
 * Window Value: 4095 (to disable windowing)
 * Counter reload value: 200
 */

#include "bsp.h"
#include "watchdog.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "userCan.h"
#ifndef DISABLE_CAN_FEATURES
#include "canHeartbeat.h"
#include AUTOGEN_DTC_HEADER_NAME(BOARD_NAME)
#endif

typedef struct TaskNode {
    uint32_t id;
    uint32_t timeoutTicks;
    uint32_t lastCheckInTicks;
    bool     isFsmTask;
    uint32_t fsmCheckInRequestTimeTicks; // Set to zero once received check in back
    FSM_Handle_Struct *fsmHandle;
    struct TaskNode *next;
} TaskNode;

#define WATCHDOGTASK_PERIOD_TICKS 5

TaskNode *tasksToWatchList = NULL;

bool signaledError = false;

/*
 * Register a task to be monitored by the watchdog
 * The watchdog will check that each task has checked in within the specified
 * period, and cause an error and reset if not
 * @id: uniqe ID to refer to the task by when tasks checks in (uniquness NOT
 * verified by watchdog task)
 * @timeoutTicks: the max interval between checkIns, in ticks
 * @isFsmTask: if the task is controlling a state machine, then instead of
 * requiring a check in interval, the watchdog task sends an event to the state
 * machine and it must respond within timeout ticks by checking in
 * @fsmHandle: (only if isFsmTask is true, set to NULL otherwise) the FSM
 * handle to send check in request to
 */
HAL_StatusTypeDef registerTaskToWatch(uint32_t id, uint32_t timeoutTicks,
                                      bool isFsmTask, FSM_Handle_Struct *fsmHandle)
{
    if (id == 0) return HAL_ERROR; // ID must be non-zero
    if (isFsmTask && fsmHandle == NULL) return HAL_ERROR;
    if (timeoutTicks == 0) return HAL_ERROR;

    TaskNode *node = (TaskNode *)pvPortMalloc(sizeof(TaskNode));
    node->id = id;
    node->timeoutTicks = timeoutTicks;
    node->lastCheckInTicks = xTaskGetTickCount();
    node->isFsmTask = isFsmTask;
    node->fsmCheckInRequestTimeTicks = 0;

    if (isFsmTask) {
        node->fsmHandle = fsmHandle;
    } else {
        node->fsmHandle = NULL;
    }

    if (tasksToWatchList == NULL) {
        tasksToWatchList = node;
    } else {
        TaskNode *n = tasksToWatchList;
        while (n->next != NULL) {n = n->next;}
        n->next = node;
    }

    return HAL_OK;
}

HAL_StatusTypeDef watchdogTaskChangeTimeout(uint32_t id, uint32_t timeoutTicks)
{
    if (id == 0) return HAL_ERROR;

    TaskNode *node = tasksToWatchList;

    while (node != NULL) {
        if (node->id == id) {
            node->timeoutTicks = timeoutTicks;
            // Treat this as a checkin as well
            // Otherwise changing timeout right before deadline might
            // sitll cause a missed deadline
            node->lastCheckInTicks = xTaskGetTickCount();

            // Cause another check in request to be sent, also to ensure reset
            // of deadline on change of timeout
            if (node->isFsmTask) {
                node->fsmCheckInRequestTimeTicks = 0;
            }
            return HAL_OK;
        }
        node = node->next;
    }

    return HAL_ERROR;
}


HAL_StatusTypeDef watchdogTaskCheckIn(uint32_t id)
{
    if (id == 0) return HAL_ERROR;

    TaskNode *node = tasksToWatchList;

    while (node != NULL) {
        if (node->id == id) {
            node->lastCheckInTicks = xTaskGetTickCount();
            return HAL_OK;
        }
        node = node->next;
    }

    return HAL_ERROR;
}

HAL_StatusTypeDef watchdogRefresh()
{
#if !BOARD_IS_WSB(BOARD_ID)
    return HAL_IWDG_Refresh(&IWDG_HANDLE);
#else
    return HAL_OK;
#endif
}

void watchdogSignalError(uint32_t id)
{
    if (!signaledError) {
        ERROR_PRINT("Watchdog error for task id %lu\n", id);

#ifndef DISABLE_CAN_FEATURES
        uint8_t data = id | (BOARD_ID << 4);
        sendDTC_FATAL_WatchdogTaskMissedCheckIn(data);
#endif
#if BOARD_TYPE != NUCLEO_F7
        handleError();
#endif
        signaledError = true;
    }
}

HAL_StatusTypeDef watchdogSendEventToFSM(FSM_Handle_Struct *fsmHandle)
{
    return fsmSendEvent(fsmHandle, WATCHDOG_REQUEST_EVENT_NUM, 0);
}

void watchdogTask(void *pvParameters)
{
    TaskNode *node = NULL;
#ifndef DISABLE_CAN_FEATURES
#if !BOARD_IS_WSB(BOARD_ID)
    uint32_t lastHeartbeatTick = 0;
#endif
#endif

#ifndef DISABLE_CAN_FEATURES
    if (canStart(&CAN_HANDLE) != HAL_OK)
    {
        ERROR_PRINT("Failed to start CAN!\n");
        Error_Handler();
    }
#endif

    // Send heartbeat on startup so it gets sent ASAP
#ifndef DISABLE_CAN_FEATURES
#if !BOARD_IS_WSB(BOARD_ID)
    sendHeartbeat();
#endif
#endif

    while (1) {
        node = tasksToWatchList;
        uint32_t curTick = xTaskGetTickCount();
        while (node != NULL) {
            if (node->isFsmTask) {
                if (node->fsmCheckInRequestTimeTicks == 0) {
                    if (watchdogSendEventToFSM(node->fsmHandle) != HAL_OK)
                    {
                        watchdogSignalError(node->id);
                    }
                    node->fsmCheckInRequestTimeTicks = curTick;
                } else {
                    // Check if we received a response to the check in request
                    if (node->lastCheckInTicks >= node->fsmCheckInRequestTimeTicks) {
                        // Check if timeout occured
                        if (node->lastCheckInTicks - node->fsmCheckInRequestTimeTicks > node->timeoutTicks) {
                            watchdogSignalError(node->id);
                        }
                        node->fsmCheckInRequestTimeTicks = 0;
                    } else {
                        // We didn't receive a response, check timeout
                        if (curTick - node->fsmCheckInRequestTimeTicks > node->timeoutTicks) {
                            watchdogSignalError(node->id);
                        }
                    }
                }
            } else {
                if (curTick - node->lastCheckInTicks > node->timeoutTicks) {
                    watchdogSignalError(node->id);
                }
            }
            node = node->next;
        }

#ifndef DISABLE_CAN_FEATURES
#if !BOARD_IS_WSB(BOARD_ID)
        if (!signaledError) {
            if (checkAllHeartbeats() != HAL_OK) {
                // checkAllHeartbeats sends DTC, so don't need to do it here
                ERROR_PRINT("Heartbeat missed!\n");
                handleError();
                signaledError = true;
            }
        }
#endif
#endif

        watchdogRefresh();

#ifndef DISABLE_CAN_FEATURES
#if !BOARD_IS_WSB(BOARD_ID)
        if (curTick - lastHeartbeatTick >= HEARTBEAT_PERIOD_TICKS) {
            sendHeartbeat();
            lastHeartbeatTick = curTick;
        }
#endif
#endif
        vTaskDelay(WATCHDOGTASK_PERIOD_TICKS);
    }
}

uint8_t wdReset = 0;
void checkForWDReset()
{
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        wdReset = 1;
    } else {
        wdReset = 0;
    }
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

void printWDResetState()
{
    if (wdReset) {
#ifndef DISABLE_CAN_FEATURES
        // Need to start CAN here since it won't be started already
        if (canStart(&CAN_HANDLE) != HAL_OK)
        {
            handleError();
        }

        sendDTC_FATAL_WatchdogReset();
#endif 
        printf("Watchdog reset occured!!!\n");
    } else {
        printf("Watchdog reset didn't occur\n");
    }
}

void handleWatchdogReset()
{
    if (wdReset) {
        while (1) {
            DEBUG_PRINT("Watchdog Reset Loop!\r\n");
            watchdogRefresh();
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

