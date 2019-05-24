#ifndef BATTERIES_H

#define BATTERIES_H

#include "freertos.h"
#include "queue.h"
#include "bsp.h"

typedef enum Charge_Notifications_t {
    CHARGE_START_NOTIFICATION,
    CHARGE_STOP_NOTIFICATION,
} Charge_Notifications_t;

extern QueueHandle_t IBusQueue;
extern QueueHandle_t VBusQueue;
extern QueueHandle_t VBattQueue;

HAL_StatusTypeDef initBusVoltagesAndCurrentQueues();
HAL_StatusTypeDef balance_cell(int cell, bool set);

#endif /* end of include guard: BATTERIES_H */
