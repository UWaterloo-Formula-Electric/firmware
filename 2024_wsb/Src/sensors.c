#include "sensors.h"

#include "encoders.h"
#include "multiSensorADC.h"
#include "stm32f4xx_hal.h"

HAL_StatusTypeDef sensors_init(void) {
    if (multi_sensor_adc_init() != HAL_OK) {
        return HAL_ERROR;
    }

//    if (encoders_init() != HAL_OK) {
//        return HAL_ERROR;
//    }

    return HAL_OK;
}
