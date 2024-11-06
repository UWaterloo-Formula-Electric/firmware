/**
 *******************************************************************************
 * @file    tempSensor.c
 * @author	Arham
 * @date    Oct 2024
 * @brief   The driver for the temperature sensing chip (P3T1755DP/Q900Z)
 *
 ******************************************************************************
 */

#include "tempSensor.h"
#include "bsp.h"
#include "debug.h"
#include "controlStateMachine.h"
#include "watchdog.h"
#include "i2c.h"
#include "can.h"
#include "stm32f7xx_hal_can.h"

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define SENSOR_ADDRESS_READ 0x91
#define SENSOR_ADDRESS_WRITE 0x90
#define CONFIG_REGISTER 0x01
#define TEMP_REGISTER 0x00
/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
uint16_t tempregister_info [2];
float temperature_reading;
/*********************************************************************************************************************/
/*------------------------------------------------Function Definition------------------------------------------------*/
/*********************************************************************************************************************/
float temperature_conversion(uint16_t temp_array[2]){
    uint16_t temp = (temp_array[0] << 4) | (temp_array[1] >> 4);
    if ((temp_array[0] & (1 << 7)) == 0){
        return (temp*0.0625);
    }else{
        temp = (temp^0xFFF) + 1;
        return (-(temp*0.0625));
    }
}
/*********************************************************************************************************************/
/*-------------------------------------------------------Task--------------------------------------------------------*/
/*********************************************************************************************************************/

void tempSensorTask(void *pvParameters)
{
    if (registerTaskToWatch(TEMP_SENSOR_TASK_ID, 2*TEMP_SENSOR_TASK_INTERVAL_MS, false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register power task with watchdog!\n");
        Error_Handler();
    }

    /* TODO: initialize the sensor (write config register if needed) */
    MX_I2C1_Init();
    //config register value would be set to 0x28 for continuous measurement, standard functioning
    HAL_I2C_Mem_Write(&hi2c1,SENSOR_ADDRESS_WRITE,CONFIG_REGISTER,I2C_MEMADD_SIZE_8BIT,&0x28,1,HAL_MAX_DELAY);
    

    TickType_t xLastWakeTime = xTaskGetTickCount();
    HAL_Delay(220);
    while (1)
    {
        /* TODO: reading and report temperature*/
        HAL_I2C_Mem_Read(&hi2c1,SENSOR_ADDRESS_READ,TEMP_REGISTER,I2C_MEMADD_SIZE_8BIT,tempregister_info,2,HAL_MAX_DELAY);
        temperature_reading = temperature_conversion(temp_register);
        
        /* Send CAN Message */
        /*alias fix='vi +'\'':wq ++ff=unix'\'' common/Scripts/generateDTC.py && vi +'\'':wq ++ff=unix'\'' common/Scripts/generateCANHeadder.py'*/

        watchdogTaskCheckIn(TEMP_SENSOR_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, TEMP_SENSOR_TASK_INTERVAL_MS);
    }
}