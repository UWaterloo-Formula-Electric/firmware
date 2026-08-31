#ifndef CAN_INJECT_H
#define CAN_INJECT_H

#include "stm32f7xx_hal.h"

// Application-level CAN message injection: hand-build frames that match
// message IDs/signal layouts in common/Data/2024CAR.dbc to spoof another
// board's messages onto the bus for testing. Since HIL isn't a DBC node,
// there are no generated sendCAN_<Msg>() helpers to call - build the
// id/payload by hand here (cross-referencing the .dbc file) and send with
// sendCanMessage() from common/Inc/userCan.h.
HAL_StatusTypeDef canInjectInit(void);

#endif /* CAN_INJECT_H */
