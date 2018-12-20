#include "sensors.h"
#include "adc.h"

uint32_t ADC_Buffer[NUM_PDU_CHANNELS];

HAL_StatusTypeDef startADCConversions()
{
    if (HAL_ADC_Start_DMA(&ADC_HANDLE, ADC_Buffer, NUM_PDU_CHANNELS) != HAL_OK)
    {
        ERROR_PRINT("Failed to start ADC DMA conversions\n");
        Error_Handler();
        return HAL_ERROR
    }

    return HAL_OK;
}

uint32_t readCurrent(PDU_Channels_t channel)
{
    uint32_t rawValue = ADC_Buffer[channel];

    return rawValue; // TODO: Add approriate scaling
}

void sensorTask(void *pvParameters)
{
    while (1)
    {

    }
}

