#include "canHeartbeat.h"
#include "debug.h"
#include AUTOGEN_DTC_HEADER_NAME(BOARD_NAME)

#define HEARTBEAT_SEND_FUNCTION CAT(CAT(sendCAN_,BOARD_NAME),_Heartbeat)

HAL_StatusTypeDef sendHeartbeat()
{
    return HEARTBEAT_SEND_FUNCTION();
}

uint32_t lastBMU_Heartbeat_ticks = 0;
uint32_t lastPDU_Heartbeat_ticks = 0;
uint32_t lastDCU_Heartbeat_ticks = 0;
uint32_t lastVCU_F7_Heartbeat_ticks = 0;

void heartbeatReceived(BoardIDs board)
{
    switch (board) {
        case ID_DCU:
            {
                lastDCU_Heartbeat_ticks = xTaskGetTickCount();
                break;
            }
        case ID_PDU:
            {
                lastPDU_Heartbeat_ticks = xTaskGetTickCount();
                break;
            }
        case ID_BMU:
            {
                lastBMU_Heartbeat_ticks = xTaskGetTickCount();
                break;
            }
        case ID_VCU_F7:
            {
                lastVCU_F7_Heartbeat_ticks = xTaskGetTickCount();
                break;
            }

        default:
            {
                ERROR_PRINT("Received heartbeat from unexpected board\n");
                break;
            }
    }
}

HAL_StatusTypeDef checkAllHeartbeats()
{
    uint32_t curTick = xTaskGetTickCount();

#if BOARD_ID != ID_DCU
    if (curTick - lastDCU_Heartbeat_ticks >= HEARTBEAT_TIMEOUT_TICKS)
    {
        sendDTC_FATAL_DCU_MissedHeartbeat();
        ERROR_PRINT("DCU Missed heartbeat check in\n");
        return HAL_ERROR;
    }
#endif

#if BOARD_ID != ID_PDU
    if (curTick - lastPDU_Heartbeat_ticks >= HEARTBEAT_TIMEOUT_TICKS)
    {
        sendDTC_FATAL_PDU_MissedHeartbeat();
        ERROR_PRINT("PDU Missed heartbeat check in\n");
        return HAL_ERROR;
    }
#endif

#if BOARD_ID != ID_BMU
    if (curTick - lastBMU_Heartbeat_ticks >= HEARTBEAT_TIMEOUT_TICKS)
    {
        sendDTC_FATAL_BMU_MissedHeartbeat();
        ERROR_PRINT("BMU Missed heartbeat check in\n");
        return HAL_ERROR;
    }
#endif

#if BOARD_ID != ID_VCU_F7
    if (curTick - lastVCU_F7_Heartbeat_ticks >= HEARTBEAT_TIMEOUT_TICKS)
    {
        sendDTC_FATAL_VCU_F7_MissedHeartbeat();
        ERROR_PRINT("VCU_F7 Missed heartbeat check in\n");
        return HAL_ERROR;
    }
#endif

    return HAL_OK;
}
