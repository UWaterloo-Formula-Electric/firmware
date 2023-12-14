#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "userInit.h"
#include "canReceive.h"
#include "processCAN.h"

/* Processes incoming CAN message*/
void process_rx_task (void * pvParameters)
{
}