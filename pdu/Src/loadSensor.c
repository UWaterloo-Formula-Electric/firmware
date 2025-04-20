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
#include "watchdog.h"

#define ENABLE_LOAD_SENSOR

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define ADC_BUFFER_LENGTH           1
#define CURRENT_SENS_CHANNELS       7
#define MUX_TRANSITION_DELAY_US     1       // Actual: 200 ns/V

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
extern uint8_t timerInitialized;        // from lvMeasure.c (share the same timer)

channelMeas_u channelCurrents;
uint32_t rawADC3Buffer[NUM_PDU_CHANNELS];

// See schematic. It was optimized for short traces
const uint8_t MUX_MAPPING[NUM_PDU_CHANNELS] = {7, 7, 5, 5, 6, 6, 3, 3, 0, 0, 1, 1, 2, 2};

volatile uint32_t adcData[ADC_BUFFER_LENGTH];
const char *channelNames[NUM_PDU_CHANNELS] = {  "Pump 1",
                                                "Pump 2",
                                                "CDU",
                                                "BMU",
                                                "WSB",
                                                "TCU",
                                                "Brake Light",
                                                "Acc. Fans",
                                                "Inverter",
                                                "Radiator",
                                                "AUX1",
                                                "AUX2",
                                                "AUX3",
                                                "AUX4"};

/*********************************************************************************************************************/
/*-----------------------------------------------------Helpers-------------------------------------------------------*/
/*********************************************************************************************************************/
/* delay function for wakeup. Use for delays < 1ms to reduce tight polling time */
void delay_us(uint16_t time_us)
{
	__HAL_TIM_SetCounter(&DELAY_TIMER,0);
	__HAL_TIM_SetAutoreload(&DELAY_TIMER,0xffff);
	HAL_TIM_Base_Start(&DELAY_TIMER);
	while(DELAY_TIMER_INSTANCE->CNT < time_us);
	HAL_TIM_Base_Stop(&DELAY_TIMER);
}

HAL_StatusTypeDef startADCConversions()
{
    if (HAL_ADC_Start_DMA(&ADC3_HANDLE, (uint32_t*)adcData, ADC_BUFFER_LENGTH) != HAL_OK)
    {
        ERROR_PRINT("Failed to start ADC DMA conversions\n");
        Error_Handler();
        return HAL_ERROR;
    }
    if (!timerInitialized)
    {
        if (HAL_TIM_Base_Start(&ADC_TIM_HANDLE) != HAL_OK) {
            ERROR_PRINT("Failed to start ADC Timer\n");
            Error_Handler();
            return HAL_ERROR;
        }
        timerInitialized = 1;
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

void printRawChannelADCVals(void) {
    for (int i = 0; i < NUM_PDU_CHANNELS; i++) {
        DEBUG_PRINT("%s: %lu\r\n", channelNames[i], rawADC3Buffer[i]);
    }
}

void printChannelCurrent(void) {
    for (int i = 0; i < NUM_PDU_CHANNELS; i++) {
        DEBUG_PRINT("%s: %f\r\n", channelNames[i], channelCurrents.meas_a[i]);
    }
}

void canPublishCurrents() {
    ChannelCurrentPump1 = channelCurrents.meas_s.Pump_1_Channel_A;
    ChannelCurrentPump2 = channelCurrents.meas_s.Pump_2_Channel_A;

    ChannelCurrentCDU = channelCurrents.meas_s.CDU_Channel_A;
    ChannelCurrentBMU = channelCurrents.meas_s.BMU_Channel_A;
    ChannelCurrentWSB = channelCurrents.meas_s.WSB_Channel_A;
    ChannelCurrentTCU = channelCurrents.meas_s.TCU_Channel_A;

    ChannelCurrentBrakeLight = channelCurrents.meas_s.Brake_Light_Channel_A;
    ChannelCurrentAccFan = channelCurrents.meas_s.ACC_Fans_Channel_A;
    ChannelCurrentInverter = channelCurrents.meas_s.INV_Channel_A;
    ChannelCurrentRadiator = channelCurrents.meas_s.Radiator_Channel_A;

    ChannelCurrentAUX1 = channelCurrents.meas_s.AUX_1_Channel_A;
    ChannelCurrentAUX2 = channelCurrents.meas_s.AUX_2_Channel_A;
    ChannelCurrentAUX3 = channelCurrents.meas_s.AUX_3_Channel_A;
    ChannelCurrentAUX4 = channelCurrents.meas_s.AUX_4_Channel_A;

    if (sendCAN_PDU_Fan_and_Pump_Current() != HAL_OK) {     // 4 channels
        ERROR_PRINT("Publish PDU_Fan_and_Pump_Current msg failed! \n");
    }
    if (sendCAN_PDU_Current_Readings() != HAL_OK) {         // 2 channels
        ERROR_PRINT("Publish PDU_Current_Readings msg failed! \n");
    }
    if (sendCAN_PDU_Current_AUX_Readings() != HAL_OK) {     // 4 channel
        ERROR_PRINT("Publish PDU_Current_AUX_Readings msg failed! \n");
    }
    if (sendCAN_PDU_Board_Channels_Current() != HAL_OK) {   // 4 channels
        ERROR_PRINT("Publish PDU_Board_Channels_Current msg failed! \n");
    }
}

// TODO: implement logic to check if the e-fuse is triggered
// void canPublishFuseStatus() {
//     for (PDU_Channels_t channel = 0; channel < NUM_PDU_CHANNELS; channel++) {
//         float channelCurrent = readCurrent(channel);
//         uint8_t fuseStatus = checkBlownFuse(channelCurrent);
//         setFuseStatusSignal(channel, fuseStatus);
//     }
//     if (sendCAN_PDU_Fuse_Status() != HAL_OK) {
//         ERROR_PRINT("Publish PDU fuse statuses failed!\n");
//     }
// }

/*********************************************************************************************************************/
/*-------------------------------------------------------Task--------------------------------------------------------*/
/*********************************************************************************************************************/

void loadSensorTask(void *pvParameters)
{
    if (registerTaskToWatch(LOAD_SENSOR_TASK_ID, 15*LOAD_SENSOR_TASK_INTERVAL_MS, false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register power task with watchdog!\n");
        Error_Handler();
    }

     // Delay to allow system to turn on
    // vTaskDelay(pdMS_TO_TICKS(100));
    TickType_t xLastWakeTime = xTaskGetTickCount();

#ifdef ENABLE_LOAD_SENSOR
    // Start Mux ADC Pin
    if (startADCConversions() != HAL_OK) {
        ERROR_PRINT("Failed to start ADC conversions\n");
        Error_Handler();
    }

    // Delay to allow system to turn on
    vTaskDelay(pdMS_TO_TICKS(100));
    // TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        // Enable Current Diagnosis
        HAL_GPIO_WritePin(I_DIAG_SENSE_EN_GPIO_Port, I_DIAG_SENSE_EN_Pin, GPIO_PIN_SET);

        // Select odd channels to read
        HAL_GPIO_WritePin(I_DIAG_SENSE_SEL_GPIO_Port, I_DIAG_SENSE_SEL_Pin, GPIO_PIN_SET);
        delay_us(SETTLING_TIME_AFTER_CHANNEL_CHANGE_HIGH_TO_LOW_US);

        // Read the odd channels
        for (uint8_t channel = 1; channel < NUM_PDU_CHANNELS; channel += 2)
        {
            selectMuxChannelForRead(MUX_MAPPING[channel]);

            // TODO: fix this timing issue (worst results with longer delays)
            // delay_us(MUX_TRANSITION_DELAY_US);
            // vTaskDelay(pdMS_TO_TICKS(300));
            vTaskDelay(pdMS_TO_TICKS(100));


            rawADC3Buffer[channel] = adcData[0];    // Only reason to save the raw ADC value is for the CLI commands

            // TODO: there is an offset at zero current (see table 22 in the datahseet). Create an adjusted reading
            // See https://docs.google.com/spreadsheets/d/1q7FbXL-useZ2Z1aKM_nWJUNIhSq9Kkbrv6WAbpqaecs/edit?usp=sharing
            channelCurrents.meas_a[channel] = ((float)rawADC3Buffer[channel]) / ADC3_TO_AMPS_DIVIDER;
        }

        // Switch to even channels
        HAL_GPIO_WritePin(I_DIAG_SENSE_SEL_GPIO_Port, I_DIAG_SENSE_SEL_Pin, GPIO_PIN_RESET);
        delay_us(SETTLING_TIME_AFTER_CHANNEL_CHANGE_HIGH_TO_LOW_US);

        // Read the even channels
        for (uint8_t channel = 0; channel < NUM_PDU_CHANNELS; channel += 2)
        {

            // TODO: fix the timing
            selectMuxChannelForRead(MUX_MAPPING[channel]);
            // delay_us(MUX_TRANSITION_DELAY_US);
            vTaskDelay(pdMS_TO_TICKS(300));

            rawADC3Buffer[channel] = adcData[0];
            channelCurrents.meas_a[channel] = ((float)rawADC3Buffer[channel] - 19) / ADC3_TO_AMPS_DIVIDER;
        }

        // Disable current diagnosis on all load channels
        // (Select pins do not matter since line is high impedance.)
        HAL_GPIO_WritePin(I_DIAG_SENSE_EN_GPIO_Port, I_DIAG_SENSE_EN_Pin, GPIO_PIN_RESET);

        // At this point, all current sens buffers must have been filled with current data. 
        canPublishCurrents();

        watchdogTaskCheckIn(LOAD_SENSOR_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, LOAD_SENSOR_TASK_INTERVAL_MS);
    }
#else
    while (1){
        watchdogTaskCheckIn(LOAD_SENSOR_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, LOAD_SENSOR_TASK_INTERVAL_MS);
    }
#endif

}
