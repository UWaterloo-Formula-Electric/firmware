#include <stdlib.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "driver/gpio.h"
#include "pduOutputs.h"

twai_message_t pdu_outputs =
{
    .identifier = RELAY_PDU_OUTPUTS,
    .extd = 1,
    .data_length_code = 2,
};

void relayPduOutputs(void * pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    static uint16_t pdu_gpio_data = 0U;

    while(1)
    {
        pdu_gpio_data = 0;
        pdu_gpio_data |= gpio_get_level(POW_AUX_PIN) << PduOutStatusBit_aux;
        pdu_gpio_data |= gpio_get_level(POW_BRAKE_LIGHT_PIN) << PduOutStatusBit_brake_light;
        pdu_gpio_data |= gpio_get_level(BATTERY_RAW_PIN) << PduOutStatusBit_battery;
        pdu_gpio_data |= gpio_get_level(POW_BMU_PIN) << PduOutStatusBit_bmu;
        pdu_gpio_data |= gpio_get_level(POW_VCU_PIN) << PduOutStatusBit_vcu;
        pdu_gpio_data |= gpio_get_level(POW_DCU_PIN) << PduOutStatusBit_dcu;
        pdu_gpio_data |= gpio_get_level(POW_MC_LEFT_PIN) << PduOutStatusBit_mc_left;
        pdu_gpio_data |= gpio_get_level(POW_MC_RIGHT_PIN) << PduOutStatusBit_mc_right;
        pdu_gpio_data |= gpio_get_level(POW_LEFT_PUMP_PIN) << PduOutStatusBit_left_pump;
        pdu_gpio_data |= gpio_get_level(POW_RIGHT_PUMP_PIN) << PduOutStatusBit_right_pump;
        pdu_gpio_data |= gpio_get_level(POW_LEFT_FAN_PIN) << PduOutStatusBit_left_fan;
        pdu_gpio_data |= gpio_get_level(POW_RIGHT_FAN_PIN) << PduOutStatusBit_right_fan;

        uint8_t byte_0 = (uint8_t)(pdu_gpio_data & 0xFF);
        uint8_t byte_1 = pdu_gpio_data >> 8;

        pdu_outputs.data[0] = byte_0;
        pdu_outputs.data[1] = byte_1;
        twai_transmit(&pdu_outputs, portMAX_DELAY);

        vTaskDelayUntil(&xLastWakeTime, RELAY_PDU_OUTPUT_INTERVAL)
    }
}