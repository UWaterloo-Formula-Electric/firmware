#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "driver/twai.h"
#include "driver/gpio.h"
#include "pduOutputs.h"

/*
static bool aux_on = false;
static bool brake_light_on = false;
static bool battery_raw_on = false;
static bool bmu_on = false;
static bool vcu_on = false;
static bool mc_left_on = false;
static bool mc_right_on = false;
static bool left_pump_on = false;
static bool right_pump_on = false;
static bool left_fan_on = false;
static bool right_fan_on = false;
*/

void (*relayPduPtr)(void*) = &relayPduOutputs;

twai_message_t pdu_outputs =
{
    .identifier = 0,
    .extd = 1,
    .data_length_code = 2,
};

void relayPduOutputs(void*)
{
    static uint16_t pdu_gpio_data;

    pdu_gpio_data |= gpio_get_level(POW_AUX) << aux;
    pdu_gpio_data |= gpio_get_level(POW_BRAKE_LIGHT) << brake_light;
    pdu_gpio_data |= gpio_get_level(BATTERY_RAW) << battery;
    pdu_gpio_data |= gpio_get_level(POW_BMU) << bmu;
    pdu_gpio_data |= gpio_get_level(POW_VCU) << vcu;
    pdu_gpio_data |= gpio_get_level(POW_DCU) << dcu;
    pdu_gpio_data |= gpio_get_level(POW_MC_LEFT) << mc_left;
    pdu_gpio_data |= gpio_get_level(POW_MC_RIGHT) << mc_right;
    pdu_gpio_data |= gpio_get_level(POW_LEFT_PUMP) << left_pump;
    pdu_gpio_data |= gpio_get_level(POW_RIGHT_PUMP) << left_pump;
    pdu_gpio_data |= gpio_get_level(POW_LEFT_FAN) << left_fan;
    pdu_gpio_data |= gpio_get_level(POW_RIGHT_FAN) << right_fan;

    uint8_t dByte0 = (uint8_t)(pdu_gpio_data & 0xFF);
    uint8_t dByte1 = pdu_gpio_data >> 8;
    
    pdu_outputs.data[0] = dByte0;
    pdu_outputs.data[1] = dByte1;
    twai_transmit(&pdu_outputs, portMAX_DELAY);
}