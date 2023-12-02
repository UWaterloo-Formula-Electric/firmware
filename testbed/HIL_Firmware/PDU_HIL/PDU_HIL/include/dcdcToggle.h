#ifndef DCDC_TOGGLE_H
#define DCDC_TOGGLE_H

#define PDU_MSG_STATUS_CAN_ID 0x8020F03
#define EXTENDED_MSG 1
#define DCDC_TOGGLE_CAN_MSG_DATA_SIZE 1 //size in bytes

void setDCDC(uint32_t level);

#endif/*DCDC_TOGGLE_H*/