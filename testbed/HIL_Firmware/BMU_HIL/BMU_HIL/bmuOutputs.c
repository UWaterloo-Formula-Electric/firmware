#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "driver/gpio.h"
#include "bmuOutputs.h"

twai_message_t bmu_outputs = 
{
    .identifier = RELAY_BMU_OUTPUTS_CAN_ID,
    .extd = EXTENDED_MSG,
    .data_length_code = BMU_OUTPUT_CAN_MSG_DATA_SIZE,
};

void relayBmuOutputs(void * pvParamaters)
{
    /* TODO: Determine what this does exactly */
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        memset(&bmu_outputs.data, 0, sizeof(bmu_outputs.data)); // clear all bits and start clean
        
        /* Set message data bits based on the GPIO pins on the BMU */
        // bmu_outputs.data[0] |= gpio_get_level(POW_BMU_PIN) << BmuOutStatusBit_Bmu;

        twai_transmit(&bmu_outputs, portMAX_DELAY); // Send CAN message
        vTaskDelayUntil(&xLastWakeTime, RELAY_BMU_OUTPUT_INTERVAL_MS);
    }

}