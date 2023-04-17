#include "canHeartbeat.h"
#include "debug.h"
#include AUTOGEN_DTC_HEADER_NAME(BOARD_NAME)

#define HEARTBEAT_SEND_FUNCTION CAT(CAT(sendCAN_,BOARD_NAME_UPPER),_Heartbeat)

HAL_StatusTypeDef sendHeartbeat()
{
    return HEARTBEAT_SEND_FUNCTION();
}

uint32_t lastBMU_Heartbeat_ticks = 0;
uint32_t lastPDU_Heartbeat_ticks = 0;
uint32_t lastDCU_Heartbeat_ticks = 0;
uint32_t lastVCU_F7_Heartbeat_ticks = 0;
// For now, we don't use this, since BMU does its own checking of the heartbeat
uint32_t lastChargeCart_Heartbeat_ticks = 0;

void heartbeatReceived(BoardIDs board)
{
    switch (board) {
        case ID_DCU:
            {
                lastDCU_Heartbeat_ticks = xTaskGetTickCountFromISR();
                break;
            }
        case ID_PDU:
            {
                lastPDU_Heartbeat_ticks = xTaskGetTickCountFromISR();
                break;
            }
        case ID_BMU:
            {
                lastBMU_Heartbeat_ticks = xTaskGetTickCountFromISR();
                break;
            }
        case ID_VCU_F7:
            {
                lastVCU_F7_Heartbeat_ticks = xTaskGetTickCountFromISR();
                break;
            }
        case ID_ChargeCart:
            {
                lastChargeCart_Heartbeat_ticks = xTaskGetTickCountFromISR();
                break;
            }
        default:
            {
                if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
                    ERROR_PRINT("Received heartbeat from unexpected board\n");
                } else {
                    printf("Received heartbeat from unexpected board\n");
                }
                break;
            }
    }
}

bool heartbeatEnabled = true;
bool DCU_heartbeatEnabled = true;
bool PDU_heartbeatEnabled = true;
bool BMU_heartbeatEnabled = true;
bool VCU_F7_heartbeatEnabled = true;

void disableHeartbeat()
{
    heartbeatEnabled = false;
}

void enableHeartbeat()
{
    heartbeatEnabled = true;
}

HAL_StatusTypeDef checkAllHeartbeats()
{
#if IS_BOARD_NUCLEO_F7
    return HAL_OK;
#endif

    if (heartbeatEnabled)
    {
        uint32_t curTick = xTaskGetTickCount();
#if BOARD_ID != ID_DCU
        if (DCU_heartbeatEnabled) {
            if (curTick - lastDCU_Heartbeat_ticks >= HEARTBEAT_TIMEOUT_TICKS)
            {
                sendDTC_FATAL_DCU_MissedHeartbeat();
                ERROR_PRINT("DCU Missed heartbeat check in\n");
                return HAL_ERROR;
            }
        }
#endif

#if BOARD_ID != ID_PDU
        if (PDU_heartbeatEnabled) {
            if (curTick - lastPDU_Heartbeat_ticks >= HEARTBEAT_TIMEOUT_TICKS)
            {
                sendDTC_FATAL_PDU_MissedHeartbeat();
                ERROR_PRINT("PDU Missed heartbeat check in\n");
                return HAL_ERROR;
            }
        }
#endif

#if BOARD_ID != ID_BMU
        if (BMU_heartbeatEnabled) {
            if (curTick - lastBMU_Heartbeat_ticks >= HEARTBEAT_TIMEOUT_TICKS)
            {
                sendDTC_FATAL_BMU_MissedHeartbeat();
                ERROR_PRINT("BMU Missed heartbeat check in\n");
                return HAL_ERROR;
            }
        }
#endif

#if BOARD_ID != ID_VCU_F7
        if (VCU_F7_heartbeatEnabled) {
            if (curTick - lastVCU_F7_Heartbeat_ticks >= HEARTBEAT_TIMEOUT_TICKS)
            {
                sendDTC_FATAL_VCU_F7_MissedHeartbeat();
                ERROR_PRINT("VCU_F7 Missed heartbeat check in\n");
                return HAL_ERROR;
            }
        }
#endif
    }

    return HAL_OK;
}


void printHeartbeatStatus()
{
    DEBUG_PRINT("HeartbeatStatus:\n");
    DEBUG_PRINT("Current Tick: %lu\n", xTaskGetTickCount());
    DEBUG_PRINT("Board\tLastReceived (ticks)\n");
    DEBUG_PRINT("PDU\t%lu\n", lastPDU_Heartbeat_ticks);
    DEBUG_PRINT("DCU\t%lu\n", lastDCU_Heartbeat_ticks);
    DEBUG_PRINT("BMU\t%lu\n", lastBMU_Heartbeat_ticks);
    DEBUG_PRINT("VCU_F7\t%lu\n", lastVCU_F7_Heartbeat_ticks);
}
