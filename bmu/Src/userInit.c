/**
  *****************************************************************************
  * @file    userInit.c
  * @author  Richard Matthews
  * @brief   Initialization before RTOS starts
  * @details Contains the userInit function, which is called before the RTOS
  * starts to allow the user to initialize modules or other things that must be
  * done before the RTOS starts
  ******************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "uwfe_debug.h"
#include "userCan.h"
#include "controlStateMachine_mock.h"
#include "controlStateMachine.h"
#include "batteries.h"

#if IS_BOARD_F7
#include "imdDriver.h"
#endif

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    signed char *pcTaskName )
{
    HAL_GPIO_WritePin(ERROR_LED_PORT, ERROR_LED_PIN, GPIO_PIN_SET);
    printf("Stack overflow for task %s\n", pcTaskName);
}

static HAL_StatusTypeDef init_HW_check_timer(void)
{
	if (HAL_TIM_PWM_Start(&HW_CHECK_HANDLE, TIM_CHANNEL_1) != HAL_OK)
	{
		ERROR_PRINT("Failed to start HW_CHECK timer\n");
		return HAL_ERROR;
	}
	return HAL_OK;
}


// This is declared with weak linkage in all Cube main.c files, and called
// before freeRTOS initializes and starts up
void userInit()
{
    /* Should be the first thing initialized, otherwise print will fail */
    if (debugInit() != HAL_OK) {
        Error_Handler();
    }

    if (uartStartReceiving(&DEBUG_UART_HANDLE) != HAL_OK)
    {
        Error_Handler();
    }

    if (canInit(&CAN_HANDLE) != HAL_OK) {
        Error_Handler();
    }

    if (initBusVoltagesAndCurrentQueues() != HAL_OK) {
        Error_Handler();
    }
 
    if (stateMachineMockInit() != HAL_OK) {
       Error_Handler();
    }

    if (controlInit() != HAL_OK) {
        Error_Handler();
    }

    if (initPackVoltageQueues() != HAL_OK) {
        Error_Handler();
    }
#if IS_BOARD_F7
    if (init_imd_measurement() != HAL_OK) {
        Error_Handler();
    }
#endif
	if (init_HW_check_timer() != HAL_OK) {
		Error_Handler();
	}

    printf("Finished user init\n");
}

