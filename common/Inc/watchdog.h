#ifndef WATCHDOG_H

#define WATCHDOG_H

#include "bsp.h"
#include "state_machine.h"

HAL_StatusTypeDef registerTaskToWatch(uint32_t id, uint32_t timeoutTicks,
                                      bool isFsmTask, FSM_Handle_Struct *fsmHandle);
HAL_StatusTypeDef watchdogTaskCheckIn(uint32_t id);
HAL_StatusTypeDef watchdogTaskChangeTimeout(uint32_t id, uint32_t timeoutTicks);
void printWDResetState();
void checkForWDReset();
void handleWatchdogReset();
HAL_StatusTypeDef watchdogRefresh();

#endif /* end of include guard: WATCHDOG_H */
