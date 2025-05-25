#ifndef RECIEVE_CAN_H
#define RECIEVE_CAN_H

#include "queue.h"

#define CAN_MSG_MAX_NUMBER 50
extern QueueHandle_t vcu_hil;

typedef struct CAN_Message {
    uint32_t id;
    uint32_t len;
    uint8_t data[8];
} CAN_Message;

#endif