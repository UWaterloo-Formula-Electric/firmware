#include <stdbool.h>
#include "driver/gpio.h"
#include "userInit.h"
#include "esp_err.h"
#include "dcdcToggle.h"
#include "driver/twai.h"

static bool DCDCSent = false;

twai_message_t message_status = 
{
    .identifier = PDU_MSG_STATUS_CAN_ID,
    .extd = EXTENDED_MSG,
    .data_length_code = DCDC_TOGGLE_CAN_MSG_DATA_SIZE,
};

void setDCDC(uint32_t level)
{
    esp_err_t fault = 0;

    fault = gpio_set_level(DC_DC_TOGGLE_PIN, level);

    if(fault != ESP_OK)
    {
        printf("Failed transmit data\n");
        DCDCSent = false;
    }
    else
    {
        DCDCSent = true;
    }

    message_status.data[1] = DCDCSent;
    twai_transmit(&message_status, portMAX_DELAY);
}