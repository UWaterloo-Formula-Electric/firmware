#include "contactorControl.h"
#include "bsp.h"
#include "bmu_can.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "watchdog.h"

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

HAL_StatusTypeDef contactorCurrentSenseInit()
{
    if(HAL_ADC_Start_DMA(&CONT_SENSE_ADC_HANDLE, contactorThermistorADCValues, NUM_CONT_THERMISTOR_INDEX) != HAL_OK) {
        ERROR_PRINT("Failed to start contactor current sense and thermistor ADC conversions.\r\n");
        Error_Handler();
        return HAL_ERROR;
    }

    return HAL_OK;
}

void contCurrentSenseTask(void *pvParameters)
{
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

        watchdogTaskCheckIn(CONT_CURRENT_SENSE_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CONTACTOR_SENSE_PERIOD));
    }
}