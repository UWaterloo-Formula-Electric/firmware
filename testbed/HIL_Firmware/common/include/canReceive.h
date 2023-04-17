#ifndef CAN_RECEIVE_H
#define CAN_RECEIVE_H

#define MAX_QUEUE_LENGTH 50

#define CAN_RX_TASK_INTERVAL 1
#define PROCCESS_RX_TASK_INTERVAL 1

twai_message_t rx_msg;
twai_message_t can_msg;

QueueHandle_t rx_vcu_hil;
QueueHandle_t rx_pdu_hil;

void can_rx_task (void * pvParameters);

#endif