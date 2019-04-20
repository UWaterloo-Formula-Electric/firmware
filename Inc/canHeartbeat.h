#ifndef CANHEARTBEAT_H

#define CANHEARTBEAT_H

#include "bsp.h"
#include "userCan.h"
#include AUTOGEN_HEADER_NAME(BOARD_NAME)
#include "boardTypes.h"

#define HEARTBEAT_PERIOD_TICKS 10
#define HEARTBEAT_TIMEOUT_TICKS 25

HAL_StatusTypeDef sendHeartbeat();
void heartbeatReceived(BoardIDs boardName);
HAL_StatusTypeDef checkAllHeartbeats();

#endif /* end of include guard: CANHEARTBEAT_H */
