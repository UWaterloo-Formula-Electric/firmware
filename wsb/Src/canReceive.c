#include "boardTypes.h"
#include "bsp.h"
#include "debug.h"

void DTC_Fatal_Callback(BoardIDs board)
{
    ERROR_PRINT("Fatal Callback received!\n");
}

void CAN_Msg_UartOverCanConfig_Callback() {
#if BOARD_ID  == ID_WSBFL
	isUartOverCanEnabled = isUartOverCanEnabled & 0x10;
#elif BOARD_ID  == ID_WSBFR
	isUartOverCanEnabled = isUartOverCanEnabled & 0x20;
#elif BOARD_ID  == ID_WSBRL
	isUartOverCanEnabled = isUartOverCanEnabled & 0x40;
#elif BOARD_ID  == ID_WSBRR
	isUartOverCanEnabled = isUartOverCanEnabled & 0x80;
#endif
}