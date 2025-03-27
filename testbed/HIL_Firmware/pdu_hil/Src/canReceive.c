#include "FreeRTOS.h"
#include "queue.h"
#include "canReceive.h"
#include "stm32f7xx_hal.h"  
#include "stm32f7xx_hal_can.h"
#include "stm32f7xx_hal_gpio.h"
#include <stdio.h>
#include "usercan.h"

QueueHandle_t pdu_hil_queue;


void CAN_Msg_BattThermistor_Callback(void *data)
{

    pdu_hil_queue = XQueueCreate(MAX_CAN_MSG_LENGTH, sizeof(CAN_Message));
    TickType_t xLastTickCount;
    
    if(xQueueSend(pdu_hil_queue, &data, portMAX_DELAY)!=pdPASS)
    {
        DEBUG_PRINT("Error Sending Theristor Value to queue");
    }
}
