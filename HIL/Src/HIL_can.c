#include "HIL_can.h"
#include "canReceive.h"

float DTC_Data = 0;
float DTC_Severity = 0;
float DTC_CODE = 0;

// HIL isn't an addressed node, so there's no per-node filter bank to set up
// the way the vehicle boards do in their generated configCANFilters().
// Accept every frame on the bus and let canReceive.c's HIL_CAN_Rx_Handler()
// decide what matters for the test being run.
void configCANFilters(CAN_HandleTypeDef *hcan)
{
    CAN_FilterTypeDef filter = {0};
    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;

    HAL_CAN_ConfigFilter(hcan, &filter);
}

HAL_StatusTypeDef init_can_driver(void)
{
    return HAL_OK;
}

HAL_StatusTypeDef parseCANData(uint32_t id, uint8_t *data)
{
    return HIL_CAN_Rx_Handler(id, data);
}

HAL_StatusTypeDef sendCAN_HIL_DTC(void)
{
    // HIL never raises DTCs - this only exists to satisfy
    // common/Src/userCan.c's sendDTCMessage()/DTC_SEND_FUNCTION() call.
    return HAL_OK;
}
