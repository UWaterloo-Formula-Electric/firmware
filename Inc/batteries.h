#ifndef BATTERIES_H

#define BATTERIES_H

#include "FreeRTOS.h"
#include "queue.h"
#include "bsp.h"

#define CELL_TIME_TO_FAILURE_ALLOWABLE (6.0)
#define CELL_DCR (0.01)
#define CELL_HEAT_CAPACITY (1034.2) //kj/kg•k
#define CELL_MASS (0.496)
#define CELL_MAX_TEMP_C (60.0)
#define CELL_OVERTEMP (CELL_MAX_TEMP_C)
#define CELL_OVERTEMP_WARNING (CELL_MAX_TEMP_C - 10)

typedef enum Charge_Notifications_t {
    CHARGE_START_NOTIFICATION,
    CHARGE_STOP_NOTIFICATION,
} Charge_Notifications_t;

extern QueueHandle_t IBusQueue;
extern QueueHandle_t VBusQueue;
extern QueueHandle_t VBattQueue;

HAL_StatusTypeDef initBusVoltagesAndCurrentQueues();
HAL_StatusTypeDef balance_cell(int cell, bool set);
HAL_StatusTypeDef getPackVoltage(float *packVoltage);
HAL_StatusTypeDef initPackVoltageQueue();
float map_range_float(float in, float low, float high, float low_out, float high_out);
HAL_StatusTypeDef setMaxChargeCurrent(float maxCurrent);

#endif /* end of include guard: BATTERIES_H */
