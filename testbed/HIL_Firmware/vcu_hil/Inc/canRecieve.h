#ifndef CAN_RECIEVE_H
#define CAN_RECIEVE_H

#include "queue.h"
#include "main.h"
#include "Dac_Driver.h"

#define CAN_MSG_MAX_NUMBER 50
extern QueueHandle_t vcu_hil;

typedef struct CAN_Message {
    uint32_t id;
    uint32_t len;
    uint8_t data[8];
} CAN_Message;

DAC_t dac1;


#endif