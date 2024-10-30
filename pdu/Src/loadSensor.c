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

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
const int enumToEnablePortMap[PDU_NUM_CHANNELS] = {
    PUMP_1_EN,
    PUMP_2_EN,
    CDU_EN,
    BMU_EN,
    WSB_EN,
    TCU_EN,
    BRAKE_LIGHT_ENABLE,
    ACC_FANS_EN,
    INVERTER_EN,
    RADIATOR_EN,
    AUX_1_EN,
    AUX_2_EN,
    AUX_3_EN,
    AUX_4_EN
};

/*********************************************************************************************************************/
/*------------------------------------------------Function Definition------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Helpers-------------------------------------------------------*/
/*********************************************************************************************************************/

void enableChannel(PDU_Channels_New channel)
{
    // Check if valid channel to enable
    if (channel < 1 || channel > 14)
    {
        // Return Channel Error
    }
    // Enable channel
    enumToEnablePortMap[channel];

    // Configure the mux based on channel
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S1_GPIO_Port, CURR_SENSE_MUX_S1_Pin, ((channel >> 0) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S2_GPIO_Port, CURR_SENSE_MUX_S2_Pin, ((channel >> 1) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CURR_SENSE_MUX_S3_GPIO_Port, CURR_SENSE_MUX_S3_Pin, ((channel >> 2) & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void disableChannel(PDU_Channels_New channel)
{    
    // Disable  channel
    HAL_GPIO_WritePin(MUX_SEL_PORT, MUX_SEL_PIN0, (channel & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    
    // What if the enabled channel is disabled but the mux is configured to that channel?
    // What is read from MUX_OUT?
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

    // Delay to allow system to turn on
    vTaskDelay(pdMS_TO_TICKS(100));

    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        /* TODO: Add logic for reading current from all channels*/

        // Example:
        // One Sequence (one read, one channel)
        // 1. Enable a load channel (selects which channel I am reading current from). Ex: Set CH_5_EN.
        // 2. At this point, current on the load channel IC (fuse should be coming through)
        // 3. Configure the MUX to the channel that is enabled
        // 4. Read from MUX Out
        // 5. Update CAN Message and publish it on the bus.
        // 5. Switch to a different channel and repeat

        static PDU_Channels_New channel = 1;
        // Enable Channel and Configure the Mux Pins
        configureChannel(channel);

        // Read from MUX out and update ADC Buffer
        HAL_ADC_Start_DMA(&ADC_HANDLE, (uint32_t*)ADC_Buffer, NUM_PDU_CHANNELS);

        // Disable Channel
        disableChannel(channel);
        channel++;
        }

        /* Send CAN Message */
        watchdogTaskCheckIn(LOAD_SENSOR_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, LOAD_SENSOR_TASK_INTERVAL_MS);
}