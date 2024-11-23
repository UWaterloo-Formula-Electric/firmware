#ifndef PROCESS_CAN_H
#define PROCESS_CAN_H

#define PROCESS_RX_TASK_INTERVAL_MS 1

//TODO: implement more sensor signals as they are integrated

/* integrated sensors
 * brake temp
 * hall effect NOTE: signal max value determined based on 2024 car specs
 */

#define WSBFL_CAN_ID 0x400080F
#define WSBFR_CAN_ID 0x400090F //todo: receiver needs mapping after signal added
#define WSBRL_CAN_ID 0x4000A0F
#define WSBRR_CAN_ID 0x4000B0F

void process_rx_task (void * pvParamters);

#endif/*PROCESS_CAN_H*/