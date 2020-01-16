#ifndef CANHEARTBEAT_H

#define CANHEARTBEAT_H

#include "bsp.h"
#include "userCan.h"
#include AUTOGEN_HEADER_NAME(BOARD_NAME)
#include "boardTypes.h"

#define HEARTBEAT_PERIOD_TICKS 1000
#define HEARTBEAT_TIMEOUT_TICKS 2500

extern bool heartbeatEnabled;
extern bool DCU_heartbeatEnabled;
extern bool PDU_heartbeatEnabled;
extern bool BMU_heartbeatEnabled;
extern bool VCU_F7_heartbeatEnabled;

HAL_StatusTypeDef sendHeartbeat();
void heartbeatReceived(BoardIDs boardName);
HAL_StatusTypeDef checkAllHeartbeats();
void enableHeartbeat();
void disableHeartbeat();
void printHeartbeatStatus();

#endif /* end of include guard: CANHEARTBEAT_H */
