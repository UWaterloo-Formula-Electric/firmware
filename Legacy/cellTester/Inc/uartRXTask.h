#ifndef __UART_RXTASK_H
#define __UART_RXTASK_H

#include "stm32f0xx_hal.h"
#include  "mainTaskEntry.h"
typedef enum {
    CellTestStatus_IDLE = 0U,
    CellTestStatus_REQUESTED,   // Start logging before drawing current
    CellTestStatus_RUNNING,     // Draw current from the cell
    CellTestStatus_LOGGING,     // After the test is done continue to log for a period of time
    CellTestStatus_ABORTED,     // A stop request was made, continue to log but cut current
} CellTestStatus_E;

CellTestStatus_E getCellTestStatus(void);
void abortCellTest(void);
void setCurrentTarget(float new_current_target);
float getCurrentTarget(void);

#endif // __UART_RXTASK_H