#ifndef CAN_RECEIVE_H
#define CAN_RECEIVE_H

#define MAX_QUEUE_LENGTH 50

#define CAN_RX_TASK_INTERVAL 1

#include "driver/twai.h"
#include "freertos/queue.h"

extern twai_message_t rx_msg;
extern twai_message_t can_msg;
extern twai_message_t doNothing_msg;

extern QueueHandle_t rx_vcu_hil;
extern QueueHandle_t rx_pdu_hil;

void can_rx_task (void * pvParameters);

#endif