#ifndef BATTERIES_H

#define BATTERIES_H

#include "freertos.h"
#include "queue.h"
#include "bsp.h"

extern QueueHandle_t IBusQueue;
extern QueueHandle_t VBusQueue;
extern QueueHandle_t VBattQueue;

HAL_StatusTypeDef initBusVoltagesAndCurrentQueues();

#endif /* end of include guard: BATTERIES_H */
