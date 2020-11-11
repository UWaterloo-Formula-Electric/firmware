#ifndef BATTERIES_H

#define BATTERIES_H

#include "FreeRTOS.h"
#include "queue.h"
#include "bsp.h"

// Used by FAN Control to determine when to turn on fans
#define CELL_MAX_TEMP_C (55.0)

typedef enum Charge_Notifications_t {
    CHARGE_START_NOTIFICATION,
    CHARGE_STOP_NOTIFICATION,
} Charge_Notifications_t;

HAL_StatusTypeDef getIBus(float *IBus);
HAL_StatusTypeDef getVBatt(float *VBatt);
HAL_StatusTypeDef getVBus(float * VBus);

HAL_StatusTypeDef initBusVoltagesAndCurrentQueues();
HAL_StatusTypeDef balance_cell(int cell, bool set);
HAL_StatusTypeDef getPackVoltage(float *packVoltage);
HAL_StatusTypeDef initPackVoltageQueue();
float map_range_float(float in, float low, float high, float low_out, float high_out);
HAL_StatusTypeDef setMaxChargeCurrent(float maxCurrent);
void setSendOnlyOneCell(int cellIdx);
void clearSendOnlyOneCell();
HAL_StatusTypeDef cliSetVBatt(float VBatt);
HAL_StatusTypeDef cliSetVBus(float VBus);
HAL_StatusTypeDef cliSetIBus(float IBus);

#endif /* end of include guard: BATTERIES_H */
