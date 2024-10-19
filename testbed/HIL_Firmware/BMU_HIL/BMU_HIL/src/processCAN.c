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
static uint16_t byte_0 = 0U;
static uint16_t byte_1 = 0U;
static uint16_t byte_2 = 0U;
static uint16_t byte_3 = 0U;
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

        byte_0 = can_msg.data[0];
        byte_1 = can_msg.data[1];
        byte_2 = can_msg.data[2];
        byte_3 = can_msg.data[3];

        switch(can_msg.identifier){
            case(BMU_HIL_GPIO_OUTPUTS_MSG):
                //Possible validation of inputs?
                //Assuming that each time a CAN message is sent, all values are sent. Might be a bad idea subject to change.

                //Bits  0-7
                gpio_set_level(CBRB_PRESS_PIN, byte_0[BMU_HIL_GPIO_OUTPUTS_CBRB_PRESS]);
                gpio_set_level(HVD_PIN, byte_0[BMU_HIL_GPIO_OUTPUTS_HVD]);
                gpio_set_level(IL_CLOSE_PIN, byte_0[BMU_HIL_GPIO_OUTPUTS_ILCLOSE]);
                gpio_set_level(TSMS_FAULT_PIN, byte_0[BMU_HIL_GPIO_OUTPUTS_TSMSFAULT]);
                gpio_set_level(AMS_RESET_PRESS_PIN, byte_0[BMU_HIL_GPIO_OUTPUTS_AMSRESETPRESS]);
                gpio_set_level(IMD_RESET_PRESS_PIN, byte_0[BMU_HIL_GPIO_OUTPUTS_IMDRESETPRESS]);
                gpio_set_level(BPSD_RESET_PRESS_PIN, byte_0[BMU_HIL_GPIO_OUTPUTS_BPSDRESETPRESS]);
                gpio_set_level(IMD_FAULT_PIN, byte_0[BMU_HIL_GPIO_OUTPUTS_IMDFAULT]);

                //IMD Status 8 bit number

                //FanTach 8 bit number
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

