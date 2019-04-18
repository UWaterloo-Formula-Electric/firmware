/*
 * In order for the watchdog to work, it needs to be enabled in CubeMX
 * The following parameters should be used, generating a WDG period of 20mS:
 * prescaler: 4
 * Window Value: 4095 (to disable windowing)
 * Counter reload value: 160
 */

#include "bsp.h"
#include "watchdog.h"
#include "freertos.h"
#include "task.h"
#include "debug.h"

typedef struct TaskNode {
    uint32_t id;
    uint32_t timeoutTicks;
    uint32_t lastCheckInTicks;
    bool     isFsmTask;
    uint32_t fsmCheckInRequestTimeTicks; // Set to zero once received check in back
    FSM_Handle_Struct *fsmHandle;
    struct TaskNode *next;
} TaskNode;

TaskNode *tasksToWatchList = NULL;

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
        tasksToWatchList->next = node;
    }

    return HAL_OK;
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
    return HAL_IWDG_Refresh(&IWDG_HANDLE);
}

void watchdogSignalError(uint32_t id)
{
    ERROR_PRINT("Watchdog error for task id %lu\n", id);
    Error_Handler();
}

HAL_StatusTypeDef watchdogSendEventToFSM(FSM_Handle_Struct *fsmHandle)
{
    return fsmSendEvent(fsmHandle, WATCHDOG_REQUEST_EVENT_NUM, 0);
}

void watchdogTask(void *pvParameters)
{
    TaskNode *node = NULL;

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
                } else {
                    if (node->lastCheckInTicks
                        - node->fsmCheckInRequestTimeTicks > node->timeoutTicks)
                    {
                        watchdogSignalError(node->id);
                    }

                    node->fsmCheckInRequestTimeTicks = 0;
                }
            } else {
                if (curTick - node->lastCheckInTicks > node->timeoutTicks) {
                    watchdogSignalError(node->id);
                }
            }
            node = node->next;
        }

        watchdogRefresh();
        vTaskDelay(5);
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
        printf("Watchdog reset occured!!!\n");
    } else {
        DEBUG_PRINT("Watchdog reset didn't occur\n");
    }
}

void handleWatchdogReset()
{
    if (wdReset) {
        while (1) {
            watchdogRefresh();
        }
    }
}

