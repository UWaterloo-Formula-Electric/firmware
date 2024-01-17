#ifndef PROCESS_CAN_H
#define PROCESS_CAN_H

#define PROCESS_RX_TASK_INTERVAL_MS 1


#define BMU_PWM_CONFIG_CAN_ID 0x40101FF

/* Define all CAN message IDs needed for the BMU below (Review Schematics) */
/* Messages that are processed on the bus will need to generate some sort of a signal for the BMU
   Eg. Thermistor on the PDU
*/

/* Process received CAN message (freeRTOS function) */
void process_rx_task (void * pvParameters);

#endif /* PROCESS_CAN_H */