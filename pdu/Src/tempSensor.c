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
#include "pdu_can.h"

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
//value of 220ms used from datasheet based on graph from conversion time against current consumption
#define SENSOR_CONVERSION_DELAY 220
#define TEMP_REGISTER_ARRAY_SIZE 2
//conversion factor used in datasheet
#define CONVERSION_FACTOR 0.0625
/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
uint8_t temp_register_info [TEMP_REGISTER_ARRAY_SIZE];
//config register value would be set to 0x28 for continuous measurement, standard functioning
uint8_t CONFIG_VALUE = 0x28;
/*********************************************************************************************************************/
/*------------------------------------------------Function Definition------------------------------------------------*/
/*********************************************************************************************************************/
/*Function converts the raw 16 bit value from the sensor to a useable value
As mentioned in the datasheet, the 4 LSBs in the data can be ignored, so this function discards those 4 bits*/
float temperature_conversion(uint8_t temp_array[2]){
    uint16_t temp = (temp_array[0] << 4) | (temp_array[1] >> 4);
    //check for whether the value is positive or negative by checking if the MSB is 1 or 0
    if ((temp_array[0] & (1 << 7)) == 0){
        return (temp*CONVERSION_FACTOR);
    }else{
        //Converting negative value to positive as needed in the datasheet using two's complement and adding 1
        temp = (temp^0xFFF) + 1;
        return (-(temp*CONVERSION_FACTOR));
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
    HAL_I2C_Mem_Write(&hi2c1,SENSOR_ADDRESS_WRITE,CONFIG_REGISTER,I2C_MEMADD_SIZE_8BIT,&CONFIG_VALUE,1,HAL_MAX_DELAY);
    

    TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskDelay(pdMS_TO_TICKS(SENSOR_CONVERSION_DELAY));
    while (1)
    {
        /* TODO: reading and report temperature*/
        HAL_I2C_Mem_Read(&hi2c1,SENSOR_ADDRESS_READ,TEMP_REGISTER,I2C_MEMADD_SIZE_8BIT,temp_register_info,2,HAL_MAX_DELAY);
        PDU_Temp = temperature_conversion(temp_register_info);
        
        /* Send CAN Message */
        sendCAN_PDU_Temperature();

        watchdogTaskCheckIn(TEMP_SENSOR_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, TEMP_SENSOR_TASK_INTERVAL_MS);
    }
}