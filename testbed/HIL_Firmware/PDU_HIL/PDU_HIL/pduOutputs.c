#include <stdlib.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "driver/gpio.h"
#include "pduOutputs.h"

void (*relayPduPtr)(void*) = &relayPduOutputs;

twai_message_t pdu_outputs =
{
    .identifier = RELAY_PDU_OUTPUTS,
    .extd = 1,
    .data_length_code = 2,
};

void relayPduOutputs(void * pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    static uint16_t pdu_gpio_data;

    while(1)
    {
        pdu_gpio_data = 0;
        pdu_gpio_data |= gpio_get_level(POW_AUX) << aux;
        pdu_gpio_data |= gpio_get_level(POW_BRAKE_LIGHT) << brake_light;
        pdu_gpio_data |= gpio_get_level(BATTERY_RAW) << battery;
        pdu_gpio_data |= gpio_get_level(POW_BMU) << bmu;
        pdu_gpio_data |= gpio_get_level(POW_VCU) << vcu;
        pdu_gpio_data |= gpio_get_level(POW_DCU) << dcu;
        pdu_gpio_data |= gpio_get_level(POW_MC_LEFT) << mc_left;
        pdu_gpio_data |= gpio_get_level(POW_MC_RIGHT) << mc_right;
        pdu_gpio_data |= gpio_get_level(POW_LEFT_PUMP) << left_pump;
        pdu_gpio_data |= gpio_get_level(POW_RIGHT_PUMP) << right_pump;
        pdu_gpio_data |= gpio_get_level(POW_LEFT_FAN) << left_fan;
        pdu_gpio_data |= gpio_get_level(POW_RIGHT_FAN) << right_fan;

        uint8_t dByte0 = (uint8_t)(pdu_gpio_data & 0xFF);
        uint8_t dByte1 = pdu_gpio_data >> 8;

        pdu_outputs.data[0] = dByte0;
        pdu_outputs.data[1] = dByte1;
        twai_transmit(&pdu_outputs, portMAX_DELAY);

        vTaskDelayUntil(&xLastWakeTime, RELAY_PDU_OUTPUT_INTERVAL)
    }
}