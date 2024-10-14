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
static uint16_t byte_3 = 0U;
static uint16_t byte_4 = 0U;
//Max 4 bytes in a message

/***********************************************
************ Function Definitions **************
************************************************/

//Dealing with CAN inputs. Take instructions from computer and set outputs.
void process_rx_task (void * pvParameters)
{
    //Make sure this task isn't starved, check-in
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1){
        //Place a message from the bmu HIL queue into the current CAN msg
        xQueueReceive(bmu_hil_queue, &can_msg, portMAX_DELAY);

        switch(can_msg.identifier){
            case(BMU_HIL_GPIO_OUTPUTS_MSG):
                break;
            case(BMU_HIL_BATTPOSNEG_MSG):
                break;
            case(BMU_HIL_HVPOSNEGOUTPUT_MSG):
                break;
            case(BMU_HIL_HVSHUNTPOSNEG_MSG):
                break;
            default:
                printf("CAN ID not recognized %ld\r\n", can_msg.identifier);
                break;

        }
    }
}

