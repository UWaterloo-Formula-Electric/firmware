#include "canInject.h"
#include "userCan.h"
#include "debug.h"

HAL_StatusTypeDef canInjectInit(void)
{
    // TODO: add functions here that build and send specific spoofed
    // messages, e.g. injectBmuHvPowerState(...), referencing
    // common/Data/2024CAR.dbc for the real message ID/signal layout, and
    // call sendCanMessage(id, length, data) (common/Inc/userCan.h) to send.
    return HAL_OK;
}
