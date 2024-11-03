/**
 *******************************************************************************
 * @file    loadSensor.c
 * @author	Rijin + Jacky Lim
 * @date    Oct 2024
 * @brief   The driver for the load channel chip (BTS70082EPAXUMA1). This will perform 
 *          current reading for each channel (7 total) and check for fault states.
 *
 ******************************************************************************
 */

#include "loadSensor.h"

#include "bsp.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "pdu_can.h"
#include "sensors.h"
#include "watchdog.h"

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define ADC_BUFFER_LENGTH 1
#define CURRENT_SENS_CHANNELS 7

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
struct ChannelCurrents
{
    uint32_t oddChannelCurrents[CURRENT_SENS_CHANNELS];
    uint32_t evenChannelCurrents[CURRENT_SENS_CHANNELS];
};

volatile uint32_t adcData[ADC_BUFFER_LENGTH];
struct ChannelCurrents currSensData;

/*********************************************************************************************************************/
/*------------------------------------------------Function Definition------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Helpers-------------------------------------------------------*/
/*********************************************************************************************************************/

HAL_StatusTypeDef startADCConversions()
{
    if (HAL_ADC_Start_DMA(&ADC3_HANDLE, (uint32_t*)adcData, ADC_BUFFER_LENGTH) != HAL_OK)
    {
        ERROR_PRINT("Failed to start ADC DMA conversions\n");
        Error_Handler();
        return HAL_ERROR;
    }
    if (HAL_TIM_Base_Start(&ADC_TIM_HANDLE) != HAL_OK) {
        ERROR_PRINT("Failed to start ADC Timer\n");
        Error_Handler();
        return HAL_ERROR;
    }
    return HAL_OK;
}

void selectMuxChannelForRead(uint8_t channel)
{
    // Configure the MUX based on channel
    GPIO_PinState muxS1 = ((channel >> 0) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    GPIO_PinState muxS2 = ((channel >> 1) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    GPIO_PinState muxS3 = ((channel >> 2) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S1_GPIO_Port, CURR_SENSE_MUX_S1_Pin, muxS1);
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S2_GPIO_Port, CURR_SENSE_MUX_S2_Pin, muxS2);
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S3_GPIO_Port, CURR_SENSE_MUX_S3_Pin, muxS3);
}

void canPublishCurrents(struct ChannelCurrents channelCurrentData) {
    // Load Channel 1
    ChannelCurrentPump1 = channelCurrentData.oddChannelCurrents[0];
    ChannelCurrentPump2 = channelCurrentData.evenChannelCurrents[0];
    
    // Load Channel 2
    ChannelCurrentCDU = channelCurrentData.oddChannelCurrents[1];
    ChannelCurrentBMU = channelCurrentData.evenChannelCurrents[1];

    // Load Channel 3
    ChannelCurrentWSB = channelCurrentData.oddChannelCurrents[2];
    ChannelCurrentTCU = channelCurrentData.evenChannelCurrents[2];

    // Load Channel 4
    ChannelCurrentBrakeLight = channelCurrentData.oddChannelCurrents[3];
    ChannelCurrentAccFan = channelCurrentData.evenChannelCurrents[3];

    // Load Channel 5
    ChannelCurrentInverter = channelCurrentData.oddChannelCurrents[4];
    ChannelCurrentRadiator = channelCurrentData.evenChannelCurrents[4];

    // Load Channel 6
    ChannelCurrentAUX1 = channelCurrentData.oddChannelCurrents[5];
    ChannelCurrentAUX2 = channelCurrentData.evenChannelCurrents[5];

    // Load Channel 7
    ChannelCurrentAUX3 = channelCurrentData.oddChannelCurrents[6];
    ChannelCurrentAUX4 = channelCurrentData.evenChannelCurrents[6];
}

/*********************************************************************************************************************/
/*-------------------------------------------------------Task--------------------------------------------------------*/
/*********************************************************************************************************************/

void loadSensorTask(void *pvParameters)
{
    if (registerTaskToWatch(LOAD_SENSOR_TASK_ID, 2*LOAD_SENSOR_TASK_INTERVAL_MS, false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register power task with watchdog!\n");
        Error_Handler();
    }

    // Start Mux ADC Pin
    if (startADCConversions() != HAL_OK) {
        ERROR_PRINT("Failed to start ADC conversions\n");
        Error_Handler();
    }

    // Delay to allow system to turn on
    vTaskDelay(pdMS_TO_TICKS(100));
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        /* Perform Current Sensing */

        // Enable Current Diagnosis
        HAL_GPIO_WritePin(I_DIAG_SENSE_EN_GPIO_Port, I_DIAG_SENSE_EN_Pin, GPIO_PIN_SET);

        // Iterate through load channels (7 in total, 2 sub channels in each)
        for (uint8_t channel = 0; channel < CURRENT_SENS_CHANNELS; channel++)
        {
            // Switch to all odd load channel currents
            HAL_GPIO_WritePin(I_DIAG_SENSE_SEL_GPIO_Port, I_DIAG_SENSE_SEL_Pin, GPIO_PIN_RESET);;
            vTaskDelay(pdMS_TO_TICKS(SETTLING_TIME_AFTER_CHANNEL_CHANGE_HIGH_TO_LOW_MS));

            selectMuxChannelForRead(channel);

            // Read from Mux Out and update odd current channel buffer
            currSensData.oddChannelCurrents[channel] = adcData[0] / ADC_TO_AMPS_DIVIDER;
            
            // Switch to all even load channel currents
            HAL_GPIO_WritePin(I_DIAG_SENSE_SEL_GPIO_Port, I_DIAG_SENSE_SEL_Pin, GPIO_PIN_SET);
            vTaskDelay(pdMS_TO_TICKS(SETTLING_TIME_AFTER_CHANNEL_CHANGE_LOW_TO_HIGH_MS));
            
            currSensData.evenChannelCurrents[channel] = adcData[0] / ADC_TO_AMPS_DIVIDER;
        }

        // Disable current diagnosis on all load channels
        // (Select pins do not matter since line is high impedance.)
        HAL_GPIO_WritePin(I_DIAG_SENSE_EN_GPIO_Port, I_DIAG_SENSE_EN_Pin, GPIO_PIN_RESET);

        /* Send CAN Message */
        // At this point, all current sens buffers must have been filled with current data. 

        canPublishCurrents(currSensData);

        watchdogTaskCheckIn(LOAD_SENSOR_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, LOAD_SENSOR_TASK_INTERVAL_MS);
    }
}