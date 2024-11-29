#ifndef PROCESS_CAN_H
#define PROCESS_CAN_H

#include <stdio.h>

#define PROCESS_RX_TASK_INTERVAL_MS 1

//TODO: implement more sensor signals as they are integrated

/* integrated sensors
 * brake temp
 * hall effect NOTE: signal max value determined based on 2024 car specs
 */

#define WSBFL_CAN_ID 0x400080F
#define WSBFR_CAN_ID 0x400090F //todo: receiver needs mapping after adding first signal
#define WSBRL_CAN_ID 0x4000A0F
#define WSBRR_CAN_ID 0x4000B0F

#define WSBFL_MSG_LENGTH 2
#define WSBFR_MSG_LENGTH 0
#define WSBRL_MSG_LENGTH 1
#define WSBRR_MSG_LENGTH 3

void process_rx_task (void * pvParamters);
void processFL(twai_message_t* can_msg);
void processFR(twai_message_t* can_msg);
void processRL(twai_message_t* can_msg);
void processRR(twai_message_t* can_msg);
void input_data(twai_message_t* can_msg, uint8_t msg_length);

//todo: add board identifier variable?

#endif/*PROCESS_CAN_H*/