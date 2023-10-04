#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/twai.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "digitalPot.h"
#include "userInit.h"

static bool PotReady = false;

twai_message_t message_status = 
{
    .identifier = PDU_MSG_STATUS_CAN_ID,
    .extd = EXTENDED_MSG,
    .data_length_code = POT_CAN_MSG_DATA_SIZE,
};

void setPotResistance (uint32_t resistance)
{
    uint8_t out_value = 0;

    if (resistance > POT_MAX_OHM)
    {
        resistance = POT_MAX_OHM;
    }
    else if (resistance < WIPER_RESISTANCE_OHM)
    {
        resistance = WIPER_RESISTANCE_OHM;
    }

    out_value = ((resistance - WIPER_RESISTANCE_OHM)*MAX_POT_DIGITAL_VALUE)/NOMINAL_RESISTANCE_OHM;

    spi_transaction_t trans = {
        .tx_data [0] = out_value,
        .length = 8,
        .flags = SPI_TRANS_USE_TXDATA,
    };

    esp_err_t fault = 0;

    fault = spi_device_transmit(pot, &trans);

    if(fault != ESP_OK)
    {
        printf("Failed transmit data\n");
        PotReady = false;
    }
    else
    {
        PotReady = true;
    }

    message_status.data[0] = PotReady;
    twai_transmit(&message_status, portMAX_DELAY);
}
