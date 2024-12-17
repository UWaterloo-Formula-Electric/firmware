#include "../Inc/dac.h"
#include "driver/dac_oneshot.h"
#include <stdio.h>
#include "esp_err.h"

esp_err_t set_dac_voltage(dac_oneshot_handle_t* channel_handle, uint16_t mV) {
    if (mV > MAX_MV) {
        printf("voltage too high");
        mV = MAX_MV;
    }

    uint8_t output_voltage = (1.0f * mV) / MAX_MV * MAX_8_BIT_VAL;

    if (dac_oneshot_output_voltage(*channel_handle, output_voltage) != ESP_OK) {
        printf("failed to set output voltage");
        return ESP_FAIL;
    }

    return ESP_OK;
}