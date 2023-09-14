#ifndef CAN_RECEIVE_H
#define CAN_RECEIVE_H

#define MAX_QUEUE_LENGTH 50

#define CAN_RX_TASK_INTERVAL_MS 1

#include "driver/twai.h"
#include "freertos/queue.h"

extern twai_message_t rx_msg;
extern twai_message_t can_msg;

extern QueueHandle_t vcu_hil_queue;
extern QueueHandle_t pdu_hil_queue;

void can_rx_task (void * pvParameters);

#endif