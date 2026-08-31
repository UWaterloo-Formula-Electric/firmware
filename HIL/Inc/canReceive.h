#ifndef CAN_RECEIVE_H
#define CAN_RECEIVE_H

#include "stm32f7xx_hal.h"

// Called from HIL_can.c's parseCANData() for every extended-ID frame HIL
// receives. Decode whatever real-board messages a given test needs to react
// to here (e.g. confirming a board under test responded correctly to an
// injected message). There's no generated per-message dispatch table like
// the vehicle boards have, since HIL isn't a DBC node - this is the single
// RX entry point.
HAL_StatusTypeDef HIL_CAN_Rx_Handler(uint32_t id, uint8_t *data);

#endif /* CAN_RECEIVE_H */
