#ifndef PROCESS_CAN_H
#define PROCESS_CAN_H

#define PROCESS_RX_TASK_INTERVAL_MS 1
#define BATTERY_THERMISTOR 0x401030f

void process_rx_task (void * pvParameters);

//End of PROCESS_CAN_H
#endif