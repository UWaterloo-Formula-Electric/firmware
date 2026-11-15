/*
 * common/Src/debug.c's CLI unconditionally references a handful of
 * canHeartbeat.h globals/functions for its "heartbeat"/"heartbeatForBoard"
 * commands. HIL doesn't participate in the vehicle's CAN heartbeat network
 * (see ../README.md) so common/Src/canHeartbeat.c isn't linked in - these
 * definitions are just enough of a stand-in to satisfy the linker.
 *
 * If HIL later needs real heartbeat monitoring, add canHeartbeat.c to
 * COMMON_LIB_SRC in board.mk and delete this file. canHeartbeat.c's
 * It also needs an HIL_Heartbeat message in 2024CAR.dbc for its
 * sendCAN_HIL_Heartbeat(). If HIL is made a receiver of a *_Heartbeat
 * message, the generated parseCANData() calls heartbeatReceived(), so an
 * empty one would have to be added here.
 */
#include "canHeartbeat.h"

bool heartbeatEnabled = false;
bool DCU_heartbeatEnabled = false;
bool PDU_heartbeatEnabled = false;
bool BMU_heartbeatEnabled = false;
bool VCU_F7_heartbeatEnabled = false;

void printHeartbeatStatus()
{
}
