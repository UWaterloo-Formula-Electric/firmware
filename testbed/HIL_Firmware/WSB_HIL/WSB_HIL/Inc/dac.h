#ifndef INC_2024_WSB_HIL_DAC_H
#define INC_2024_WSB_HIL_DAC_H
#include "driver/dac_oneshot.h"
#include <stdio.h>
#include "esp_err.h"

#define MAX_MV 3300
#define MAX_8_BIT_VAL 255

esp_err_t set_dac_voltage(dac_oneshot_handle_t* channel_handle, uint16_t mV);

#endif //INC_2024_WSB_HIL_DAC_H
