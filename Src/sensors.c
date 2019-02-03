#include "bsp.h"
#include "freertos.h"
#include "task.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "sensors.h"
#include "adc.h"

// TODO: Find this value
#define FUSE_BLOWN_MIN_CURRENT_AMPS 0.001

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

bool checkBlownFuse(float channelCurrent)
{
    return channelCurrent >= FUSE_BLOWN_MIN_CURRENT_AMPS;
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

            if (checkBlownFuse(channelCurrent)) {
                ERROR_PRINT("Channel %d has a blown fuse\n", i);
                fsmSendEventUrgent(&mainFsmHandle, MN_EV_HV_CriticalFailure, 1000);
            }

            DEBUG_PRINT("Channel %i: %f A\n", i, channelCurrent);
            // TODO: Log current
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_PERIOD_MS));
    }
}

