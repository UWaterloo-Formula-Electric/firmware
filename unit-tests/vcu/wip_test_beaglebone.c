#include "unity.h"

#include "gpio.h"
#include "beaglebone.h"

#include <assert.h>

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"
#include "fake uwfe_debug.h"

#define BB_TASK_PERIOD 10000

void test_beagleboneOff()
{
    /*PP_BB_DISABLE;
    PP_5V0_DISABLE;

    return HAL_OK;*/
    TEST_ASSERT(1 == 1);
}

void test_beaglebonePower()
{
    /*if (enable) {
        PP_5V0_ENABLE;
        vTaskDelay(100);
        PP_BB_ENABLE;
    } else {
        //TODO: send shutdown message to BB
        PP_BB_DISABLE;
        vTaskDelay(100);
        PP_5V0_DISABLE;
    }

    return HAL_OK;*/
    TEST_ASSERT(1 == 1);
}

void test_bbTask()
{
    /*HAL_StatusTypeDef rc;

    // power on beaglebone
    // Make sure you have the beaglebone battery connected when powering the
    // beaglebone on, to ensure it safely shuts down when the VCU is powered
    // off

    DEBUG_PRINT("Powering on beaglebone, make sure beaglebone battery is plugged in!!\n");
    rc = beaglebonePower(true);
    if (rc != HAL_OK) {
        ERROR_PRINT("Failed to power on beaglebone!\n");
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        vTaskDelayUntil(&xLastWakeTime, BB_TASK_PERIOD);
    }*/
    TEST_ASSERT(1 == 1);
}
