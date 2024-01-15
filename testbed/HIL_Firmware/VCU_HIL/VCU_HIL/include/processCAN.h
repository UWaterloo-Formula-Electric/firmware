#ifndef PROCESS_CAN_H
#define PROCESS_CAN_H

#define PROCESS_RX_TASK_INTERVAL 1
#define BRAKE_POS 0x404020F
#define BRAKE_PRES_RAW 0x403020F
#define THROTTLE_A 0x401020F
#define THROTTLE_B 0x402020F
#define STEER_RAW 0x405020F


void process_rx_task (void * pvParameters);

//End of PROCESS_CAN_H
#endif