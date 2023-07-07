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

twai_message_t message_status = 
{
    .identifier = 0x8020F03,
    .extd = 1,
    .data_length_code = 1,
};

int setPotResistance (uint32_t resistance)
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

    //print statements for debugging, will be removed
    printf("resistance is %ld\r\n", resistance);

    out_value = ((resistance - WIPER_RESISTANCE_OHM)*MAX_DIGITAL_VALUE)/NOMINAL_RESISTANCE_OHM;

    printf("digi value is %d\r\n", out_value);

    spi_transaction_t trans = {
        .tx_data [0] = out_value,
        .length = 8,
        .flags = SPI_TRANS_USE_TXDATA,
    };

    esp_err_t fault = 0;

    fault = spi_device_transmit(pot, &trans);
    message_status.data[0] = 1;
    if(fault != ESP_OK)
    {
        printf("Failed transmit data\n");
        return ESP_FAIL;
    }

    twai_transmit(&message_status, portMAX_DELAY);
    return ESP_OK;

}
