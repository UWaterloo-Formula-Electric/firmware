#include "sensors.h"

#include "multiSensorADC.h"
#include "stm32f4xx_hal.h"

HAL_StatusTypeDef sensors_init(void) {
    if (multi_sensor_adc_init() != HAL_OK)
        return HAL_ERROR;

    // Add your other sensor peripheral inits like starting timers, encoders, etc. over here
    return HAL_OK;
}
