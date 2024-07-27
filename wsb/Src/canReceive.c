#include "boardTypes.h"
#include "bsp.h"
#include "uwfe_debug.h"
#if BOARD_ID  == ID_WSBFL
	#include "wsbfl_can.h"
#elif BOARD_ID  == ID_WSBFR
	#include "wsbfr_can.h"
#elif BOARD_ID  == ID_WSBRL
	#include "wsbrl_can.h"
#elif BOARD_ID  == ID_WSBRR
	#include "wsbrr_can.h"
#endif

void DTC_Fatal_Callback(BoardIDs board)
{
    ERROR_PRINT("Fatal Callback received!\n");
}

void CAN_Msg_UartOverCanConfig_Callback() {
#if BOARD_ID  == ID_WSBFL
	isUartOverCanEnabled = UartOverCanConfigSignal & 0x10;
#elif BOARD_ID  == ID_WSBFR
	isUartOverCanEnabled = UartOverCanConfigSignal & 0x20;
#elif BOARD_ID  == ID_WSBRL
	isUartOverCanEnabled = UartOverCanConfigSignal & 0x40;
#elif BOARD_ID  == ID_WSBRR
	isUartOverCanEnabled = UartOverCanConfigSignal & 0x80;
#endif
}