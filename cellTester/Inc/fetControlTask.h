#ifndef __FET_CONTROL_TASK_H__
#define __FET_CONTROL_TASK_H__

#include "stm32f0xx_hal.h"

void fetControlTask(void const * argument);
HAL_StatusTypeDef fetInit();

#endif // __FET_CONTROL_TASK_H__