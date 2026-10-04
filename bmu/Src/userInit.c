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
#include "debug.h"
#include "userCan.h"
#include "canHeartbeat.h"
#include "controlStateMachine_mock.h"
#include "controlStateMachine.h"
#include "batteries.h"
#include "state_of_charge.h"

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

// VCU_Data (has BrakePercent) is addressed to the PDU, so the generated CAN filters drop it. The firmware BSPD needs it
#define VCU_DATA_CAN_ID 0x8100302
#define VCU_DATA_CAN_FILTER_BANK 4 // Generated filters use banks 0-3
#define CAN_EXT_ID_MASK 0x1FFFFFFF

static HAL_StatusTypeDef initVcuDataCanFilter(void)
{
    // Filter registers hold the ID left aligned to 32 bits, with the IDE bit at bit 2
    uint32_t filterID = (VCU_DATA_CAN_ID << 3) | CAN_ID_EXT;
    uint32_t filterMask = (CAN_EXT_ID_MASK << 3) | CAN_ID_EXT;

    CAN_FilterTypeDef sFilterConfig = {0};
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = (filterID >> 16) & 0xFFFF;
    sFilterConfig.FilterIdLow = filterID & 0xFFFF;
    sFilterConfig.FilterMaskIdHigh = (filterMask >> 16) & 0xFFFF;
    sFilterConfig.FilterMaskIdLow = filterMask & 0xFFFF;
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.FilterBank = VCU_DATA_CAN_FILTER_BANK;
    sFilterConfig.SlaveStartFilterBank = 0; // Same as the generated filters

    if (HAL_CAN_ConfigFilter(&CAN_HANDLE, &sFilterConfig) != HAL_OK) {
        ERROR_PRINT("Failed to add VCU_Data CAN filter\n");
        return HAL_ERROR;
    }
    return HAL_OK;
}

void initTSSI()
{
    TSSI_GREEN_ON;
    TSSI_RED_OFF;
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

    if (FIRMWARE_BSPD && initVcuDataCanFilter() != HAL_OK) {
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
    
	if (init_HW_check_timer() != HAL_OK) {
		Error_Handler();
	}

    if (initSOC() != HAL_OK) {
        Error_Handler();
    }

    initTSSI();

    if (CHARGE_CART_MODE) {
        // No PDU, VCU or DCU on the charge cart to send heartbeats
        disableHeartbeat();
        printf("Charge cart mode: heartbeat checks off\n");
    }

    printf("Finished user init\n");
}

