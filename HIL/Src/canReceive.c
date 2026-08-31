#include "canReceive.h"
#include "debug.h"

HAL_StatusTypeDef HIL_CAN_Rx_Handler(uint32_t id, uint8_t *data)
{
    // TODO: decode/react to specific message IDs here, cross-referencing
    // common/Data/2024CAR.dbc for their layout.
    (void)id;
    (void)data;
    return HAL_OK;
}
