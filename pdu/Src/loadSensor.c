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
#define CHANNEL_BUFFER_LENGTH 1
#define ENABLE_CHANNEL_FLAG 1
#define DISABLE_CHANNEL_FLAG 0

// ToDo: Find the correct mapping value
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

volatile uint32_t adcData[CHANNEL_BUFFER_LENGTH];

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

void enableDisableChannel(Load_Channels channel, int enableDisableFlag) {
    if (channel <= MAX_LOAD_CHANNELS) {
        switch (channel) {
            case Pump_1_Channel:
                enableDisableFlag ? PUMP_1_EN : PUMP_1_DISABLE;
                break;
            case Pump_2_Channel:
                enableDisableFlag ? PUMP_2_EN : PUMP_2_DISABLE;
                break;
            case CDU_Channel:
                enableDisableFlag ? CDU_EN : CDU_DISABLE;
                break;
            case BMU_Channel:
                enableDisableFlag ? BMU_EN : BMU_DISABLE;
                break;
            case WSB_1_Channel:
                enableDisableFlag ? WSB_EN : WSB_DISABLE;
                break;
            case TCU_Channel:
                enableDisableFlag ? TCU_EN : TCU_DISABLE;
                break;
            case Brake_Light_Channel:
                enableDisableFlag ? BRAKE_LIGHT_ENABLE : BRAKE_LIGHT_DISABLE;
                break;
            case Acc_Fans_Channel:
                enableDisableFlag ? ACC_FANS_EN : ACC_FANS_DISABLE;
                break;
            case Inverter_Channel:
                enableDisableFlag ? INVERTER_EN : INVERTER_DISABLE;
                break;
            case Radiator_Channel:
                enableDisableFlag ? RADIATOR_EN : RADIATOR_DISABLE;
                break;
            case Aux_1_Channel:
                enableDisableFlag ? AUX_1_EN : AUX_1_DISABLE;
                break;
            case Aux_2_Channel:
                enableDisableFlag ? AUX_2_EN : AUX_2_DISABLE;
                break;
            case Aux_3_Channel:
                enableDisableFlag ? AUX_3_EN : AUX_3_DISABLE;
                break;
            case Aux_4_Channel:
                enableDisableFlag ? AUX_4_EN : AUX_4_DISABLE;
                break;
            default:
                DEBUG_PRINT("Channel not handled by code\n");
                break;
        }
    }
}

void configureChannel(Load_Channels channel)
{
    // Enable Channel
    enableDisableChannel(channel, ENABLE_CHANNEL_FLAG);

    // Configure the MUX based on channel
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S1_GPIO_Port, CURR_SENSE_MUX_S1_Pin, ((channel >> 0) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S2_GPIO_Port, CURR_SENSE_MUX_S2_Pin, ((channel >> 1) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S3_GPIO_Port, CURR_SENSE_MUX_S3_Pin, ((channel >> 2) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void disableChannel(Load_Channels channel)
{    
    // Disable Channel
    enableDisableChannel(channel, DISABLE_CHANNEL_FLAG);
    
    // Comments:
    // What if the enabled channel is disabled but the mux is configured to that channel?
    // What is read from MUX_OUT?
}

void canPublishCurrent(Load_Channels channel, float current) {
    // for (Load_Channels channel = 0; channel < MAX_NUM_CHANNELS; channel++) {
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
        /* TODO: Add logic for reading current from all channels*/
        
        static Load_Channels channel = 1;

        // Enable Channel and Configure the Mux Pins
        configureChannel(channel);

        // Read from MUX out and update ADC Buffer
        // channelCurrent = adcData[0] / ADC_TO_AMPS_DIVIDER;

        float channelCurrent = adcData[0] / ADC_TO_AMPS_DIVIDER;

        // Disable Channel
        disableChannel(channel);

        // Check if all channels have been iterated, else continue to next channel
        channel = (channel == MAX_LOAD_CHANNELS) ? 1 : channel + 1;

        /* Send CAN Message */
        canPublishCurrent(channel, channelCurrent);

        watchdogTaskCheckIn(LOAD_SENSOR_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, LOAD_SENSOR_TASK_INTERVAL_MS);
    }
}