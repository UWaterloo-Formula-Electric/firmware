#include "bsp.h"
#include "freertos.h"
#include "task.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "sensors.h"
#include "adc.h"

uint32_t ADC_Buffer[NUM_PDU_CHANNELS];

HAL_StatusTypeDef startADCConversions()
{
    if (HAL_ADC_Start_DMA(&ADC_HANDLE, ADC_Buffer, NUM_PDU_CHANNELS) != HAL_OK)
    {
        ERROR_PRINT("Failed to start ADC DMA conversions\n");
        Error_Handler();
        return HAL_ERROR;
    }

    return HAL_OK;
}

float readCurrent(PDU_Channels_t channel)
{
    uint32_t rawValue = ADC_Buffer[channel];

    return rawValue; // TODO: Add approriate scaling
}

float readBusVoltage()
{
    uint32_t rawValue = ADC_Buffer[LV_Voltage];

    return rawValue; // TODO: Add approriate scaling
}

float readBusCurrent()
{
    uint32_t rawValue = ADC_Buffer[LV_Current];

    return rawValue; // TODO: Add approriate scaling
}

void sensorTask(void *pvParameters)
{
    if (startADCConversions() != HAL_OK) {
        ERROR_PRINT("Failed to start ADC conversions\n");
        Error_Handler();
    }

    while (1)
    {
        if (readBusVoltage() <= LOW_VOLTAGE_LIMIT_VOLTS) {
            fsmSendEventUrgent(&mainFsmHandle, MN_EV_LV_Cuttoff, 1000);
        }

        if (readBusCurrent() >= LV_MAX_CURRENT_AMPS) {
            ERROR_PRINT("LV Current exceeded max value\n");
            // TODO: Should we do anything?
        }

        DEBUG_PRINT("Channel currents:\n");
        for (int i=0; i<NUM_PDU_CHANNELS; i++) {
            float channelCurrent = readCurrent(i);
            DEBUG_PRINT("1: %f\n", channelCurrent);
            // TODO: Log current
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_PERIOD_MS));
    }
}

