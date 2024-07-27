#include "canReceiveCommon.h"

#include "userCan.h"
#include "bsp.h"
#include "uwfe_debug.h"
#include "boardTypes.h"

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#if BOARD_ID  == ID_BMU
	#include "bmu_can.h"
#elif BOARD_ID  == ID_DCU
	#include "dcu_can.h"
#elif BOARD_ID  == ID_VCU_F7
	#include "vcu_F7_can.h"
#elif BOARD_ID  == ID_PDU
	#include "pdu_can.h"
#elif BOARD_ID  == ID_WSBFL
	#include "wsbfl_can.h"
#elif BOARD_ID  == ID_WSBFR
	#include "wsbfr_can.h"
#elif BOARD_ID  == ID_WSBRL
	#include "wsbrl_can.h"
#elif BOARD_ID  == ID_WSBRR
	#include "wsbrr_can.h"
#elif BOARD_ID  == ID_TCU
	#include "tcu_can.h"
#endif

void CAN_Msg_UartOverCanTx_Callback() 
{
	if (!isUartOverCanEnabled)
	{
		return;
	}
	uint8_t data = (uint8_t) UartOverCanTX;
	BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR( uartRxQueue, &data, &xHigherPriorityTaskWoken );

    if( xHigherPriorityTaskWoken )
    {
        portYIELD();
    }
}
