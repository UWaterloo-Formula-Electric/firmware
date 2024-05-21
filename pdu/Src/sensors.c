#include "bsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "FreeRTOS_CLI.h"
#include "debug.h"
#include "sensors.h"
#include "adc.h"
#include <stdbool.h>
#include "pdu_can.h"
#include "pdu_dtc.h"
#include "watchdog.h"
#include "bsp.h"

#define LV_BATTERY_VOLTAGE_LOW_DTC_PERIOD_MS 60000
#define READY_FOR_PUBLISH(TICKCOUNT, LAST_TIME, INTERVAL) ((TICKCOUNT) - (LAST_TIME) >= pdMS_TO_TICKS(INTERVAL))

volatile uint32_t ADC_Buffer[NUM_PDU_CHANNELS];

const char *channelNames[] = {  "Fan Right",
                            "DCU",
                            "MC Left",
                            "Pump Left",
                            "Fan Left",
                            "VCU",
                            "Brake Light",
                            "AUX",
                            "LV",
                            "LV",
                            "MC Right",
                            "Pump Right",
                            "BMU",
                            "WSB"};

void setFuseStatusSignal(PDU_Channels_t channel, uint8_t status) {
    if (channel < NUM_PDU_CHANNELS) {
        switch (channel) {
            case Fan_Right_Channel:
                FuseBlownFanRight = status;
                break;
            case DCU_Channel:
                FuseBlownDCUChannel = status;
                break;
            case MC_Left_Channel:
                FuseBlownMCLeft = status;
                break;
            case Pump_Left_Channel:
                FuseBlownPumpLeft = status;
                break;
            case Fan_Left_Channel:
                FuseBlownFanLeft = status;
                break;
            case VCU_Channel:
                FuseBlownVCUChannel = status;
                break;
            case Brake_Light_Channel:
                FuseBlownBrakeLight = status;
                break;
            case AUX_Channel:
                FuseBlownAUX = status;
                break;
            case MC_Right_Channel:
                FuseBlownMCRight = status;
                break;
            case Pump_Right_Channel:
                FuseBlownPumpRight = status;
                break;
            case BMU_Channel:
                FuseBlownBMUChannel = status;
                break;
            case WSB_Channel:
                FuseBlownWSBChannel = status;
                break;
            case LV_Current:
                break;
            case LV_Voltage:
                break;
            default:
                DEBUG_PRINT("Channel not handled by code\n");
                break;
        }
    }
}

void setChannelCurrentSignal(PDU_Channels_t channel, float current) {
    if (channel < NUM_PDU_CHANNELS) {
        switch (channel) {
            case Fan_Right_Channel:
                ChannelCurrentFanRight = current;
                break;
            case DCU_Channel:
                ChannelCurrentDCU = current;
                break;
            case MC_Left_Channel:
                ChannelCurrentMCLeft = current;
                break;
            case Pump_Left_Channel:
                ChannelCurrentPumpLeft = current;
                break;
            case Fan_Left_Channel:
                ChannelCurrentFanLeft = current;
                break;
            case VCU_Channel:
                ChannelCurrentVCU = current;
                break;
            case Brake_Light_Channel:
                ChannelCurrentBrakeLight = current;
                break;
            case AUX_Channel:
                ChannelCurrentAUX = current;
                break;
            case MC_Right_Channel:
                ChannelCurrentMCRight = current;
                break;
            case Pump_Right_Channel:
                ChannelCurrentPumpRight = current;
                break;
            case BMU_Channel:
                ChannelCurrentBMU = current;
                break;
            case WSB_Channel:
                ChannelCurrentWSB = current;
                break;
            case LV_Current:
            case LV_Voltage:
                break;
            default:
                DEBUG_PRINT("Channel not handled by code\n");
                break;
        }
    }
}

HAL_StatusTypeDef startADCConversions()
{
#ifndef MOCK_ADC_READINGS
    if (HAL_ADC_Start_DMA(&ADC_HANDLE, (uint32_t*)ADC_Buffer, NUM_PDU_CHANNELS) != HAL_OK)
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

void printRawADCVals() {
    for (int i=0; i < NUM_PDU_CHANNELS; i++) {
        if (i == LV_Voltage) {
            DEBUG_PRINT("Bus Voltage: %lu\n", ADC_Buffer[i]);
        } else if (i == LV_Current) {
            DEBUG_PRINT("Bus current: %lu\n", ADC_Buffer[i]);
        } else {
            DEBUG_PRINT("Channel %d, %s: %lu\n", i,channelNames[i], ADC_Buffer[i]);
        }
    }
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
    if (registerTaskToWatch(SENSOR_TASK_ID, 2*pdMS_TO_TICKS(SENSOR_READ_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register sensor task with watchdog!\n");
        Error_Handler();
    }

    if (startADCConversions() != HAL_OK) {
        ERROR_PRINT("Failed to start ADC conversions\n");
        Error_Handler();
    }

    // Delay to allow adc readings to start
    vTaskDelay(100);
    TickType_t lastLvBattLowSent = xTaskGetTickCount();

    while (1)
    {
        CurrentBusLV = readBusCurrent() * 1000;
        VoltageBusLV = readBusVoltage() * 1000;
        if (sendCAN_LV_Bus_Measurements() != HAL_OK)
        {
            ERROR_PRINT("Failed to send bus measurements on can!\n");
        }

        if (readBusCurrent() >= LV_MAX_CURRENT_AMPS) {
            ERROR_PRINT("LV Current exceeded max value\n");
            // TODO: Should we do anything?
        }

        if ((xTaskGetTickCount() - lastLvBattLowSent) > LV_BATTERY_VOLTAGE_LOW_DTC_PERIOD_MS && VoltageBusLV < 11.0f)
        {
            sendDTC_WARNING_LV_Battery_Low();
            lastLvBattLowSent = xTaskGetTickCount();
        }

        watchdogTaskCheckIn(SENSOR_TASK_ID);
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_PERIOD_MS));
    }
}

void canPublishCurrent() {
    for (PDU_Channels_t channel = 0; channel < NUM_PDU_CHANNELS; channel++) {
        setChannelCurrentSignal(channel, readCurrent(channel));
    }
    if (sendCAN_PDU_Fan_and_Pump_Current() != HAL_OK) {
        ERROR_PRINT("Publish PDU_Fan_and_Pump_Current msg failed! \n");
    }
    if (sendCAN_PDU_Current_Readings() != HAL_OK) {
        ERROR_PRINT("Publish PDU_Current_Readings msg failed! \n");
    }
    if (sendCAN_PDU_Board_Channels_Current() != HAL_OK) {
        ERROR_PRINT("Publish PDU_Board_Channels_Current msg failed! \n");
    }
}

void canPublishFuseStatus() {
    for (PDU_Channels_t channel = 0; channel < NUM_PDU_CHANNELS; channel++) {
        float channelCurrent = readCurrent(channel);
        uint8_t fuseStatus = checkBlownFuse(channelCurrent);
        setFuseStatusSignal(channel, fuseStatus);
    }
    if (sendCAN_PDU_Fuse_Status() != HAL_OK) {
        ERROR_PRINT("Publish PDU fuse statuses failed!\n");
    }
}

void canPublishPowerStates() {
    DC_DC_ON = CHECK_DC_DC_ON_PIN;
    BMGR1_State = CHECK_BMGR_GPIO1_PIN_STATE;
    BMGR2_State = CHECK_BMGR_GPIO2_PIN_STATE;
    BMGR3_State = CHECK_BMGR_GPIO3_PIN_STATE;
    if (sendCAN_PDU_Power_States() != HAL_OK) {
        ERROR_PRINT("Failed to send the PDU power states on the CAN bus!\n");
    }
}

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
    canPublishCurrent();
    canPublishCarState();
}

void canPublishPeriod5s() {
    canPublishPowerStates();
}

void canPublishPeriod10s() {
    canPublishFuseStatus();
}

void canPublishTask(void const * argument) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t LastPublishTime_1s = 0;
    uint32_t LastPublishTime_5s = 0;
    uint32_t LastPublishTime_10s = 0;

    while (1) {
        // Publish current measurements every 1 second
        uint32_t currentTickCount = xTaskGetTickCount();
        if (READY_FOR_PUBLISH(currentTickCount, LastPublishTime_1s, 1000)) {
            canPublishPeriod1s();
            LastPublishTime_1s = currentTickCount;
        }

        // Publish the PDU power states every 5 seconds
        if (READY_FOR_PUBLISH(currentTickCount, LastPublishTime_5s, 5000)) {
            canPublishPeriod5s();
            LastPublishTime_5s = currentTickCount;
        }

        // Publish if fuse is blown every 10 seconds
        if (READY_FOR_PUBLISH(currentTickCount, LastPublishTime_10s, 10000)) {
            canPublishPeriod10s();
            LastPublishTime_10s = currentTickCount;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(PDU_PUBLISH_PERIOD_MS));
    }
}
