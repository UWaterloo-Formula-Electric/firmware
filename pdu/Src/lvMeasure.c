/**
 *******************************************************************************
 * @file    lvMeasure.c
 * @author	Jacky Lim
 * @date    Oct 2024
 * @brief   Cycles through all 6 channels on ADC_1, process and report those values
 *
 ******************************************************************************
 */

#include "bsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "FreeRTOS_CLI.h"
#include "debug.h"
#include "lvMeasure.h"
#include "adc.h"
#include <stdbool.h>
#include "pdu_can.h"
#include "pdu_dtc.h"
#include "watchdog.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define LV_BUS_VOLTAGE_LOW_DTC_PERIOD_MS 60000
#define READY_FOR_PUBLISH(TICKCOUNT, LAST_TIME, INTERVAL) ((TICKCOUNT) - (LAST_TIME) >= pdMS_TO_TICKS(INTERVAL))

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
volatile uint32_t ADC1_Buffer[NUM_ADC1_CHANNELS];
busMeas_u busResults;
uint8_t timerInitialized = 0;

const char *diagnosticChannelNames[] = {  
                                "Bus Current [A]",
                                "Bus Voltage [V]",
                                "Lipo Voltage [V]",
                                "DCDC Current [A]",
                                "DCDC Voltage [V]",
                                "Lipo Thermistor [C]" };

/*********************************************************************************************************************/
/*-----------------------------------------------------Helpers-------------------------------------------------------*/
/*********************************************************************************************************************/
HAL_StatusTypeDef startADC1Conversions()
{
#ifndef MOCK_ADC_READINGS
    if (HAL_ADC_Start_DMA(&ADC1_HANDLE, (uint32_t*)ADC1_Buffer, NUM_ADC1_CHANNELS) != HAL_OK)
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

#else
    // Init ADC buffer to a known value (0)
    memset(ADC1_Buffer, 0, sizeof(ADC1_Buffer));
#endif

    return HAL_OK;
}

void printRawADCVals(void) {
    for (int i = 0; i < NUM_ADC1_CHANNELS; i++) {
        DEBUG_PRINT("%s: %lu\r\n", diagnosticChannelNames[i], ADC1_Buffer[i]);
    }
}

/* For I_MAIN and I_DCDC */
float readCurrent(PDU_ADC1_Channels_t channel)
{
    // TODO: the circuit is being replaced for rev 2. Fix till when rev 2 is done.
    return (((float)ADC1_Buffer[channel] - 2242.0) / ADC1_TO_AMPS_DIVIDER);
    // return (((float)ADC1_Buffer[channel]) / ADC1_TO_AMPS_DIVIDER);
}

/* For V_Main_Channel, V_Lipo_Channel, V_DCDC_Channel */
float readVoltage(PDU_ADC1_Channels_t channel)
{
    return (ADC1_Buffer[channel] / ADC1_TO_VOLTS_DIVIDER); 
}

/* This is primarily used to check whether a channel is blown. Should only be called for channels
   that are activated. Should be moved to loadSensor.c */
bool checkBlownFuse(float channelCurrent)
{
    // TODO: verify behavior when the e-fuse is triggered
    return channelCurrent <= FUSE_BLOWN_MIN_CURRENT_AMPS;
}

/*********************************************************************************************************************/
/*-----------------------------------------------lvMeasure Task------------------------------------------------------*/
/*********************************************************************************************************************/
void lvMeasureTask(void *pvParameters)
{
    if (registerTaskToWatch(SENSOR_TASK_ID, 2*pdMS_TO_TICKS(SENSOR_READ_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register sensor task with watchdog!\n");
        Error_Handler();
    }

    if (startADC1Conversions() != HAL_OK) {
        ERROR_PRINT("Failed to start ADC conversions\n");
        Error_Handler();
    }

    /* Delay to allow adc readings to start */
    vTaskDelay(100);
    TickType_t lastLvInfoSent = xTaskGetTickCount();

    while (1)
    {
        /* Read voltage and current in the LV bus */
        busResults.meas_s.I_Main_Channel_A = readCurrent(I_Main_Channel);
        CurrentBusLV = busResults.meas_s.I_Main_Channel_A;

        busResults.meas_s.V_Main_Channel_V = readVoltage(V_Main_Channel);
        VoltageBusLV = busResults.meas_s.V_Main_Channel_V;

        PowerBusLV = (uint64_t)(busResults.meas_s.I_Main_Channel_A * busResults.meas_s.V_Main_Channel_V); // Watt

        if (sendCAN_LV_Bus_Measurements() != HAL_OK)
        {
            ERROR_PRINT("Failed to send bus measurements on can!\n");
        }

        if (readCurrent(I_Main_Channel) >= LV_MAX_CURRENT_AMPS) {
            ERROR_PRINT("Error: LV bus current exceeded max value of 30A. Check Fuse!\n");
            // TODO: Should we do anything?
        }

        /* Low battery warning. Period of 1 min. */
        /* Note: With a DCDC, this would imply both sources are at low voltage */
        if ((xTaskGetTickCount() - lastLvInfoSent) > LV_BUS_VOLTAGE_LOW_DTC_PERIOD_MS && VoltageBusLV < 11.0f)
        {
            sendDTC_WARNING_LV_Battery_Low();
            lastLvInfoSent = xTaskGetTickCount();
        }

        watchdogTaskCheckIn(SENSOR_TASK_ID);
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_PERIOD_MS));
    }
}

/*********************************************************************************************************************/
/*-----------------------------------------------CAN Helpers---------------------------------------------------------*/
/*********************************************************************************************************************/

void canPublishDCDCInfo(void)
{
    busResults.meas_s.I_DCDC_Channel_A = readCurrent(I_DCDC_Channel);
    CurrentOutputDCDC = busResults.meas_s.I_DCDC_Channel_A;

    busResults.meas_s.V_DCDC_Channel_V = readVoltage(V_DCDC_Channel);
    VoltageOutputDCDC = busResults.meas_s.V_DCDC_Channel_V;

    /* Check if the DCDC is in used */
    Status_DCDC = (CurrentOutputDCDC > MIN_BUS_CURRENT);        // 1 = DCDC in used, 0 = not in used

    if (sendCAN_PDU_DCDC_Status() != HAL_OK) {
        ERROR_PRINT("Publish DCDC status failed!\n");
    }
}

void canPublishLipoInfo(void)
{
    /* The DCDC can charge the LIPO. Therefore, to accurately report the current, we need to know charging status.
       TODO: add logic to detect if the LIPO is being charged. For 2025, there is no DCDC so report bus current
    */
    LIPO_Current = readCurrent(I_Main_Channel);
    LIPO_Voltage = readVoltage(V_Lipo_Channel);
    LIPO_Temp = 69.42;       // TODO: should be replaced after knowing the thermistor

    if (sendCAN_PDU_Lipo_Info() != HAL_OK) {
        ERROR_PRINT("Publish LIPO status failed!\n");
    }
}

void canPublishChargingStatus(void)
{
    // TODO: add logic to detect whether the DCDC is charging the LIPO or not

    Charging_Lipo_Status = 0; // 1 = charging and 0 = not charging the LIPO

    if (sendCAN_PDU_Charging_Lipo_Status() != HAL_OK) {
        // ERROR_PRINT("Publish LIPO Charging status failed!\n");
    }
}

// void canPublishPDUTemp(void)
// {
//     // TODO: this should be moved to tempSensor.c
//     PDU_Temp = 69.42;

//     if (sendCAN_PDU_Temperature() != HAL_OK) {
//         // ERROR_PRINT("Publish PDU temperature failed!\n");
//     }
// }

void canPublishCarState() {
    // Boolean car states for telemetry dashboard
    CarStateIsLV = fsmGetState(&mainFsmHandle) == STATE_Boards_On;
    CarStateIsHV = HV_Power_State;
    CarStateIsEM = fsmGetState(&mainFsmHandle) == STATE_Motors_On;
    if (sendCAN_PDU_Car_State() != HAL_OK) {
        ERROR_PRINT("Failed to send the car state on the CAN bus!\n");
    }
}

void canPublishPeriod1s() {
    canPublishCarState();
}

void canPublishPeriod5s() {
    canPublishDCDCInfo();
    canPublishLipoInfo();
    // canPublishPDUTemp();    // TODO: move to tempSensor.c
}

void canPublishPeriod10s() {
    canPublishChargingStatus();
}

/*********************************************************************************************************************/
/*-----------------------------------------------canPublish Task-----------------------------------------------------*/
/*********************************************************************************************************************/
void canPublishTask(void const * argument) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t LastPublishTime_1s = 0;
    uint32_t LastPublishTime_5s = 0;
    uint32_t LastPublishTime_10s = 0;

    while (1) {
        uint32_t currentTickCount = xTaskGetTickCount();

        // Publish the car state every 1 second
        if (READY_FOR_PUBLISH(currentTickCount, LastPublishTime_1s, 1000)) {
            canPublishPeriod1s();
            LastPublishTime_1s = currentTickCount;
        }

        // Publish the PDU power states every 5 seconds
        if (READY_FOR_PUBLISH(currentTickCount, LastPublishTime_5s, 5000)) {
            canPublishPeriod5s();
            LastPublishTime_5s = currentTickCount;
        }

        // Publish the charging state every 10 seconds
        if (READY_FOR_PUBLISH(currentTickCount, LastPublishTime_10s, 10000)) {
            canPublishPeriod10s();
            LastPublishTime_10s = currentTickCount;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(PDU_PUBLISH_PERIOD_MS));
    }
}
