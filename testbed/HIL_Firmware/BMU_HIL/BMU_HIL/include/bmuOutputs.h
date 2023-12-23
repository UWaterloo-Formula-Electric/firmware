#ifndef BMU_OUTPUTS_H
#define BMU_OUTPUTS_H

#define RELAY_BMU_OUTPUT_INTERVAL_MS 100

// TODO: Verify the following BMU outputs
#define FAN_PWM_PIN 1
#define FAN_TACH_PIN 2
#define CS_HV_NEG_OUTPUT_PIN 6
#define CS_HV_POS_OUTPUT_PIN 7
#define IMD_STATUS_PIN 9
#define IMD_FAULT_PIN 10
#define HVD_PIN 11
#define IL_CLOSE_PIN 12
#define TSMS_FAULT_PIN 13
#define CBRB_PRESS_PIN 14
#define CS_HV_SHUNT_NEG_PIN 15
#define CS_HV_SHUNT_POS_PIN 16
#define CS_BATT_NEG_PIN 17
#define CS_BATT_POS_PIN 18

#define RELAY_BMU_OUTPUTS_CAN_ID 0x8030F03 // temporary value (copied from the PDU)
#define EXTENDED_MSG 1
#define BMU_OUTPUT_CAN_MSG_DATA_SIZE 2 //size in bytes

#define BYTE_SIZE 8

void relayBmuOutputs(void * pvParameters);

#endif/*BMU_OUTPUTS_H*/
