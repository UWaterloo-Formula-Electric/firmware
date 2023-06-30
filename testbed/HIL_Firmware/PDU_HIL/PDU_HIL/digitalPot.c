#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/twai.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "digitalPot.h"
#include "userInit.h"

//Digital pot being used https://www.analog.com/media/en/technical-documentation/data-sheets/AD5260_5262.pdf

int setPotResitance (uint32_t resistance)
{
    uint8_t out_value = 0;

    if (resistance > POT_MAX)
    {
        resistance = POT_MAX;
    }
    else if (resistance < WIPER_RESISTANCE_OHM)
    {
        resistance = WIPER_RESISTANCE_OHM;
    }

    out_value = ((resistance - WIPER_RESISTANCE_OHM)/NOMINAL_RESISTANCE_OHM)*MAX_DIGITAL_VALUE;

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
        return ESP_FAIL;
    }

    return ESP_OK;

}
