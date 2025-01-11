#include <math.h>

#include "contactorControl.h"
#include "bsp.h"
#include "bmu_can.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "watchdog.h"

#define ADC_RESOLUTION_BITS 12
#define ADC_FS_RANGE 3.3f
#define ADC_LSB ADC_FS_RANGE/(float)pow(2, ADC_RESOLUTION_BITS)
#define VOLTAGE_TO_CURRENT_GAIN 0.8f
#define ADC_TO_CURRENT ADC_LSB/VOLTAGE_TO_CURRENT_GAIN

uint32_t contactorThermistorADCValues[NUM_CONT_THERMISTOR_INDEX] = {0};

/**
 * @brief Control negative contactor
 *
 * @param state The state to set contactor to
 */
void setNegContactor(ContactorState_t state)
{
    DEBUG_PRINT("%s negative contactor\n", state==CONTACTOR_CLOSED?"Closing":"Opening");
    if (state==CONTACTOR_CLOSED) CONT_NEG_CLOSE;
    else if (state == CONTACTOR_OPEN) CONT_NEG_OPEN;
}

/**
 * @brief Control positive contactor
 *
 * @param state The state to set contactor to
 */
void setPosContactor(ContactorState_t state)
{
    DEBUG_PRINT("%s positive contactor\n", state==CONTACTOR_CLOSED?"Closing":"Opening");

    if (state==CONTACTOR_CLOSED) CONT_POS_CLOSE;
    else if (state == CONTACTOR_OPEN) CONT_POS_OPEN;
}

/**
 * @brief Control precharge discharge contactor
 *
 * @param state The state to set contactor to. Note for PCDC contactor closed
 * means precharge mode, open means discharge mode
 */
void setPrechargeContactor(ContactorState_t state)
{
    DEBUG_PRINT("%s precharge contactor\n", state==CONTACTOR_CLOSED?"Closing":"Opening");

    if (state==CONTACTOR_CLOSED) PCDC_PC;
    else if (state == CONTACTOR_OPEN) PCDC_DC;
}

/**
 * @brief Opens all contactors (safe state)
 */
void openAllContactors()
{
    setPosContactor(CONTACTOR_OPEN);
    setNegContactor(CONTACTOR_OPEN);
    setPrechargeContactor(CONTACTOR_OPEN);
}

/**
 * @brief Initialize contactor current sesning using DMA
 */
HAL_StatusTypeDef contactorCurrentSenseInit()
{
    if(HAL_ADC_Start_DMA(&CONT_SENSE_ADC_HANDLE, contactorThermistorADCValues, NUM_CONT_THERMISTOR_INDEX) != HAL_OK) {
        ERROR_PRINT("Failed to start contactor current sense and thermistor ADC conversions.\r\n");
        Error_Handler();
        return HAL_ERROR;
    }

    return HAL_OK;
}

void getContactorCurrent (float * posContCurrent, float * negContCurrent)
{
    *posContCurrent = contactorThermistorADCValues[CONT_POS_SENSE_INDEX]*ADC_TO_CURRENT;
    *negContCurrent = contactorThermistorADCValues[CONT_NEG_SENSE_INDEX]*ADC_TO_CURRENT;
}

void contCurrentSenseTask(void *pvParameters)
{
    static float posCurrent = 0.0f;
    static float negCurrent = 0.0f;

    if (registerTaskToWatch(CONT_CURRENT_SENSE_TASK_ID, 2*pdMS_TO_TICKS(CONTACTOR_SENSE_PERIOD), false, NULL) != HAL_OK) {
        ERROR_PRINT("Failed to register contactor current sense task with watchdog!\n");
        Error_Handler();
    }

    if (contactorCurrentSenseInit() != HAL_OK) {
       ERROR_PRINT("Failed to init contactor current sense task\n");
       Error_Handler();
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1){

        getContactorCurrent(&posCurrent, &negCurrent);

        DEBUG_PRINT("Pos cont current: %f\r\n", posCurrent);
        DEBUG_PRINT("Neg cont current: %f\r\n", negCurrent);

        watchdogTaskCheckIn(CONT_CURRENT_SENSE_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CONTACTOR_SENSE_PERIOD));
    }
}