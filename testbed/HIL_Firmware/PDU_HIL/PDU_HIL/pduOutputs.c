#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "driver/gpio.h"
#include "pduOutputs.h"

twai_message_t pdu_outputs =
{
    .identifier = RELAY_PDU_OUTPUTS_CAN_ID,
    .extd = EXTENDED_MSG,
    .data_length_code = PDU_OUTPUT_CAN_MSG_DATA_SIZE,
};

void relayPduOutputs(void * pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        // clear bits
        memset(&pdu_outputs.data, 0, sizeof(pdu_outputs.data));

        pdu_outputs.data[0] |= gpio_get_level(POW_AUX_PIN) << PduOutStatusBit_Aux;
        pdu_outputs.data[0] |= gpio_get_level(POW_BRAKE_LIGHT_PIN) << PduOutStatusBit_BrakeLight;
        pdu_outputs.data[0] |= gpio_get_level(BATTERY_RAW_PIN) << PduOutStatusBit_Battery;
        pdu_outputs.data[0] |= gpio_get_level(POW_BMU_PIN) << PduOutStatusBit_Bmu;
        pdu_outputs.data[0] |= gpio_get_level(POW_VCU_PIN) << PduOutStatusBit_Vcu;
        pdu_outputs.data[0] |= gpio_get_level(POW_DCU_PIN) << PduOutStatusBit_Dcu;
        pdu_outputs.data[0] |= gpio_get_level(POW_MC_LEFT_PIN) << PduOutStatusBit_McLeft;
        pdu_outputs.data[0] |= gpio_get_level(POW_MC_RIGHT_PIN) << PduOutStatusBit_McRight;
        pdu_outputs.data[1] |= gpio_get_level(POW_LEFT_PUMP_PIN) << (PduOutStatusBit_LeftPump % BYTE_SIZE);
        pdu_outputs.data[1] |= gpio_get_level(POW_RIGHT_PUMP_PIN) << (PduOutStatusBit_RightPump % BYTE_SIZE);
        pdu_outputs.data[1] |= gpio_get_level(POW_LEFT_FAN_PIN) << (PduOutStatusBit_LeftFan % BYTE_SIZE);
        pdu_outputs.data[1] |= gpio_get_level(POW_RIGHT_FAN_PIN) << (PduOutStatusBit_RightFan % BYTE_SIZE);

        twai_transmit(&pdu_outputs, portMAX_DELAY);

        vTaskDelayUntil(&xLastWakeTime, RELAY_PDU_OUTPUT_INTERVAL_MS);
    }
}
