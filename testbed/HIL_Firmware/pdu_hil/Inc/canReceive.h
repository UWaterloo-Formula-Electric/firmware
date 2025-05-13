#ifndef CANRECEIVE_H
#define CANRECEIVE_H

#include "FreeRTOS.h"
#include "queue.h"
#include "stm32f7xx_hal.h"  
#include "stm32f7xx_hal_can.h"
#include "stm32f7xx_hal_gpio.h"
#include <stdio.h>

extern QueueHandle_t pdu_hil_queue;
#define MAX_CAN_MSG_LENGTH 50

#endif //CANRECEIVE_H