/***********************************
************ INCLUDES **************
************************************/
//Standard Includes
#include <stdio.h>
#include <stdlib.h>

//ESP-IDF Includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/twai.h"

//Other Includes
#include "userInit.h"
#include "dac.h"
#include "canReceive.h"
#include "processCAN.h"

/********************************
************ Bytes **************
*********************************/
static uint16_t byte_1 = 0U;
static uint16_t byte_2 = 0U;

/***********************************************
************ Function Definitions **************
************************************************/

//Dealing with CAN inputs. Take instructions from computer and set outputs.
void process_rx_task (void * pvParameters)
{
    //TODO: Fill this Task.
}

