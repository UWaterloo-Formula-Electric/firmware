#include "main.h"
#include "RelayPDUOutput.h"
#include "stm32f7xx_hal.h"  
#include "stm32f7xx_hal_can.h"
#include "stm32f7xx_hal_gpio.h"
#include <stdio.h>
#include "pdu_hil_can.h"


void RelayPDUOutput_Init()
{
    PowAux = HAL_GPIO_ReadPin(PWR_AUX_GPIO_Port, PWR_AUX_Pin);
    PowBrakeLight = HAL_GPIO_ReadPin(PWR_BRAKELIGHT_GPIO_Port, PWR_BRAKELIGHT_Pin);
    BatteryRaw = HAL_GPIO_ReadPin(PWR_BATT_RAW_GPIO_Port, PWR_BATT_RAW_Pin);
    PowMcLeft = HAL_GPIO_ReadPin(PWR_LEFT_MC_GPIO_Port, PWR_LEFT_MC_Pin);
    PowMcRight = HAL_GPIO_ReadPin(PWR_RIGHT_MC_GPIO_Port, PWR_RIGHT_MC_Pin);
    PowLeftPump = HAL_GPIO_ReadPin(PWR_LEFT_PUMP_GPIO_Port, PWR_LEFT_PUMP_Pin);
    PowRightPump = HAL_GPIO_ReadPin(PWR_RIGHT_PUMP_GPIO_Port, PWR_RIGHT_PUMP_Pin);
    PowLeftFan = HAL_GPIO_ReadPin(PWR_LEFT_FAN_GPIO_Port, PWR_LEFT_FAN_Pin);
    PowRightFan = HAL_GPIO_ReadPin(PWR_RIGHT_FAN_GPIO_Port, PWR_RIGHT_FAN_Pin);
    PowBmu = HAL_GPIO_ReadPin(PWR_BMU_GPIO_Port, PWR_BMU_Pin);
    PowVcu = HAL_GPIO_ReadPin(PWR_VCU_GPIO_Port, PWR_VCU_Pin);
    
    if(sendCAN_PDU_RelayOutputs()!= HAL_OK)
    {
        DEBUG_PRINT("Error Transimitting PDU Output");
    }

}