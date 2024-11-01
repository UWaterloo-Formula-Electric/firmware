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
#include "debug.h"
#include "controlStateMachine.h"
#include "watchdog.h"

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define MAX_LOAD_CHANNELS 14
#define ADC_BUFFER_LENGTH 1
#define CHANNEL_DATA_BUFFER_LEN 7

// Todo: Find the correct mapping value
#define ADC_TO_AMPS_DIVIDER 2.012

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/

typedef enum PDU_Channels_t {
    Pump_1_Channel = 1,  // Channel 1
    Pump_2_Channel,      // Channel 2
    CDU_Channel,         // Channel 3
    BMU_Channel,         // Channel 4
    WSB_1_Channel,       // Channel 5
    TCU_Channel,         // Channel 6
    Brake_Light_Channel, // Channel 7
    Acc_Fans_Channel,    // Channel 8
    Inverter_Channel,    // Channel 9
    Radiator_Channel,    // Channel 10
    Aux_1_Channel,       // Channel 11
    Aux_2_Channel,       // Channel 12
    Aux_3_Channel,       // Channel 13
    Aux_4_Channel,       // Channel 14
} Load_Channels;

struct ChannelCurrents
{
    uint32_t oddChannelCurrents[CHANNEL_DATA_BUFFER_LEN];
    uint32_t evenChannelCurrents[CHANNEL_DATA_BUFFER_LEN];
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
    if (HAL_ADC_Start_DMA(&ADC3_HANDLE, (uint32_t*)adcData, MAX_LOAD_CHANNELS) != HAL_OK)
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

void selectMuxChannelForRead(Load_Channels channel)
{
    // Configure the MUX based on channel
    GPIO_PinState muxS1 = (((channel-1) >> 0) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    GPIO_PinState muxS2 = (((channel-1) >> 1) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    GPIO_PinState muxS3 = (((channel-1) >> 2) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S1_GPIO_Port, CURR_SENSE_MUX_S1_Pin, muxS1);
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S2_GPIO_Port, CURR_SENSE_MUX_S2_Pin, muxS2);
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S3_GPIO_Port, CURR_SENSE_MUX_S3_Pin, muxS3);
}

void canPublishCurrents(struct ChannelCurrents channelCurrentData) {
    // Todo: Populate CAN Messages to be sent on the bus.
    // for (Load_Channels channel = 0; channel < MAX_LOAD_CHANNELS; channel++) {
    //     switch (channel) {
    //         // case Pump_1_Channel:
    //         //     ChannelCurrentFanRight = current;
    //     }
    // }
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

        static Load_Channels channel = 1;

        // Iterate through load channels
        for (int index = 0; index < CHANNEL_DATA_BUFFER_LEN; index++)
        {
            // Switch to all odd load channel currents
            HAL_GPIO_WritePin(I_DIAG_SENSE_SEL_GPIO_Port, I_DIAG_SENSE_SEL_Pin, GPIO_PIN_RESET);;
            vTaskDelay(pdMS_TO_TICKS(SETTLING_TIME_AFTER_CHANNEL_CHANGE_HIGH_TO_LOW_MS));

            selectMuxChannelForRead(channel);

            // Read from Mux Out and update odd current channel buffer
            currSensData.oddChannelCurrents[index] = adcData[0] / ADC_TO_AMPS_DIVIDER;
            
            // Switch to all even load channel currents
            HAL_GPIO_WritePin(I_DIAG_SENSE_SEL_GPIO_Port, I_DIAG_SENSE_SEL_Pin, GPIO_PIN_SET);
            vTaskDelay(pdMS_TO_TICKS(SETTLING_TIME_AFTER_CHANNEL_CHANGE_LOW_TO_HIGH_MS));
            
            currSensData.evenChannelCurrents[index] = adcData[0] / ADC_TO_AMPS_DIVIDER;

            // Check if all 7 load channels have been iterated, else continue to next channel
            channel = (channel == CHANNEL_DATA_BUFFER_LEN) ? 1 : channel + 1;
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