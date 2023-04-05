#ifndef MAIN_H
#define MAIN_H

#define MAX_QUEUE_LENGTH 50

#define CAN_RX_TASK_INTERVAL 1
#define PROCCESS_RX_TASK_INTERVAL 1

uint8_t dbyte0;
uint16_t dbyte1;
uint16_t dbyte2;

void proccess_rx_task (void * pvParameters);

twai_message_t rx_msg;
twai_message_t can_msg;

QueueHandle_t rx_vcu_hil;

void can_rx_task (void * pvParameters);
void taskRegister (void);

//end of MAIN_H
#endif