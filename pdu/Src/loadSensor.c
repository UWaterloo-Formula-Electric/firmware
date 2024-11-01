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
#define SELECT_ODD_CHANNEL_FLAG 0
#define SELECT_EVEN_CHANNEL_FLAG 1
#define CHANNEL_BUFFER_LENGTH 7 // may need to make this more clear?

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

struct ChannelCurrents
{
    uint32_t oddChannelCurrents[CHANNEL_BUFFER_LENGTH];
    uint32_t evenChannelCurrents[CHANNEL_BUFFER_LENGTH];
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

void enableCurrentSens() 
{
    // Turn on current diagnosis
    HAL_GPIO_WritePin(I_DIAG_SENSE_EN_GPIO_Port, I_DIAG_SENSE_EN_Pin, GPIO_PIN_SET);
}

void disableCurrentSens() 
{
    // Disable current diagnosis on all load channels, select pins do not matter since line is 
    // high impedance.
    HAL_GPIO_WritePin(I_DIAG_SENSE_EN_GPIO_Port, I_DIAG_SENSE_EN_Pin, GPIO_PIN_RESET);
}

void switchLoadChannelsForRead(int selectChannelFlag) 
{
    // Switch load channels to read current from
    // If flag = 1, select even load channels (2, 4, 6...) else (1, 3, 5..)
    HAL_GPIO_WritePin(I_DIAG_SENSE_SEL_GPIO_Port, I_DIAG_SENSE_SEL_Pin, 
                     (selectChannelFlag == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void selectMuxChannelForRead(Load_Channels channel)
{
    // Configure the MUX based on channel
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S1_GPIO_Port, CURR_SENSE_MUX_S1_Pin, ((channel >> 0) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S2_GPIO_Port, CURR_SENSE_MUX_S2_Pin, ((channel >> 1) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S3_GPIO_Port, CURR_SENSE_MUX_S3_Pin, ((channel >> 2) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// void disableChannel(Load_Channels channel)
// {    
//     // Disable Channel
//     enableDisableChannel(channel, DISABLE_CHANNEL_FLAG);
    
//     // Comments:
//     // What if the enabled channel is disabled but the mux is configured to that channel?
//     // What is read from MUX_OUT?
// }

void canPublishCurrent(Load_Channels channel, float current) {
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
        /* TODO: Add logic for reading current from all channels*/

        // Logic for Current Sensing
        // Enable current diagnosis
        // Read from channel 0 and channel 1 at the same time. 
        // Read data from all 14 channels at once and populate the ADC buffers.
        // There is a delay between when you switch from one channel to another that needs to be included
        // in the firmware.
        // 0 to 1, 1 to 0 (different delays each time as per datasheet)
        
        // Enable Current Diagnosis
        enableCurrentSens();

        static Load_Channels channel = 1;

        for (int index = 0; index < CHANNEL_BUFFER_LENGTH; index++)
        {
            // Switch to all odd load channel currents
            switchLoadChannelsForRead(SELECT_ODD_CHANNEL_FLAG);

            vTaskDelay(SETTLING_TIME_AFTER_CHANNEL_CHANGE_HIGH_TO_LOW_MS);

            // Set up Channel Mux
            selectMuxChannelForRead(channel);

            // Read from Mux Out and update ADC Buffer
            currSensData.oddChannelCurrents[index] = adcData[0] / ADC_TO_AMPS_DIVIDER;
            
            // Switch to all even load channel currents
            switchLoadChannelsForRead(SELECT_EVEN_CHANNEL_FLAG);

            // Wait for switching load channel to take effect then return back to task
            vTaskDelay(SETTLING_TIME_AFTER_CHANNEL_CHANGE_LOW_TO_HIGH_MS);
            
            currSensData.evenChannelCurrents[index] = adcData[0] / ADC_TO_AMPS_DIVIDER;

            // Check if all channels have been iterated, else continue to next channel
            channel = (channel == CHANNEL_BUFFER_LENGTH) ? 1 : channel + 1;
        }

        // Disable Current Diagnosis
        disableCurrentSens();

        /* Send CAN Message */
        // canPublishCurrent(channel, channelCurrent);

        watchdogTaskCheckIn(LOAD_SENSOR_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, LOAD_SENSOR_TASK_INTERVAL_MS);
    }
}