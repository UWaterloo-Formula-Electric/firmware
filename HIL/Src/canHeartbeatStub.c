/*
 * common/Src/debug.c's CLI unconditionally references a handful of
 * canHeartbeat.h globals/functions for its "heartbeat"/"heartbeatForBoard"
 * commands. HIL doesn't participate in the vehicle's CAN heartbeat network
 * (see ../README.md) so common/Src/canHeartbeat.c isn't linked in - these
 * definitions are just enough of a stand-in to satisfy the linker.
 *
 * If HIL later needs real heartbeat monitoring, add canHeartbeat.c to
 * COMMON_LIB_SRC in board.mk, delete this file, and add ID_HIL to
 * common/Inc/boardTypes.h first (canHeartbeat.c's checkAllHeartbeats() needs
 * a board ID and hand-written DTC macros to report a missing peer).
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
