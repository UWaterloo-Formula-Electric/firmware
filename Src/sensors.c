#include "bsp.h"
#include "freertos.h"
#include "task.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "sensors.h"
#include "adc.h"
#include "watchdog.h"

volatile uint32_t ADC_Buffer[NUM_PDU_CHANNELS];

HAL_StatusTypeDef startADCConversions()
{
#ifndef MOCK_ADC_READINGS
    if (HAL_ADC_Start_DMA(&ADC_HANDLE, ADC_Buffer, NUM_PDU_CHANNELS) != HAL_OK)
    {
        ERROR_PRINT("Failed to start ADC DMA conversions\n");
        Error_Handler();
        return HAL_ERROR;
    }
#else
    // Init to reasonable values
    for (int i=0; i < NUM_PDU_CHANNELS; i++) {
        if (i == LV_Voltage) {
            ADC_Buffer[i] = 12 * ADC_TO_VOLTS_DIVIDER;
        } else if (i == LV_Current) {
            ADC_Buffer[i] = 20 * ADC_TO_AMPS_DIVIDER;
        } else {
            ADC_Buffer[i] = 1.5 * ADC_TO_AMPS_DIVIDER;
        }
    }
#endif

    return HAL_OK;
}

float readCurrent(PDU_Channels_t channel)
{
    uint32_t rawValue = ADC_Buffer[channel];

    return rawValue / ADC_TO_AMPS_DIVIDER;
}

float readBusVoltage()
{
    uint32_t rawValue = ADC_Buffer[LV_Voltage];

    return rawValue / ADC_TO_VOLTS_DIVIDER;
}

float readBusCurrent()
{
    uint32_t rawValue = ADC_Buffer[LV_Current];

    return rawValue / ADC_TO_AMPS_DIVIDER;
}

bool checkBlownFuse(float channelCurrent)
{
    return channelCurrent <= FUSE_BLOWN_MIN_CURRENT_AMPS;
}

void sensorTask(void *pvParameters)
{
    if (registerTaskToWatch(4, 2*pdMS_TO_TICKS(SENSOR_READ_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register sensor task with watchdog!\n");
        Error_Handler();
    }

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

        /*DEBUG_PRINT("Channel currents:\n");*/
        for (int i=0; i<NUM_PDU_CHANNELS; i++) {
            if (i == LV_Current || i == LV_Voltage) {
                continue;
            }

            float channelCurrent = readCurrent(i);
            /*DEBUG_PRINT("Channel %i: %f A\n", i, channelCurrent);*/

            if (checkBlownFuse(channelCurrent)) {
                ERROR_PRINT("Channel %d has a blown fuse\n", i);
                fsmSendEventUrgent(&mainFsmHandle, MN_EV_HV_CriticalFailure, 1000);
            }

            // TODO: Log current
        }

        watchdogTaskCheckIn(4);
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_PERIOD_MS));
    }
}

