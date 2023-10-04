#ifndef PROCESS_CAN_H
#define PROCESS_CAN_H

#define PROCESS_RX_TASK_INTERVAL_MS 1
#define BATTERY_THERMISTOR_CAN_ID 0x401030f

void process_rx_task (void * pvParameters);

#endif/*PROCESS_CAN_H*/
