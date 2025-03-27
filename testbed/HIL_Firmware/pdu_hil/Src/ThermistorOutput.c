#include "FreeRTOS.h"
#include "queue.h"
#include "canReceive.h"
#include "stm32f7xx_hal.h"  
#include "stm32f7xx_hal_can.h"
#include "stm32f7xx_hal_gpio.h"
#include "ThermistorOutput.h"
#include <stdio.h>
#include "Dac_Driver.h"
#include "Dac_Driver.c"

I2C_HandleTypeDef *hi2c;
uint16_t voltage;


void ThermistorOutput_Init()
{
    // read from the queue the data
    if(xQueueReceive(pdu_hil_queue, &can_rx, portMAX_DELAY) != pdPASS)
    {
        DEBUG_PRINT("Failed to recieve CAN thermistor data");
    }

    // get the first 18 bits, this is ohms
    resistance = can_rx->BattThermistor|mask;
    Dac_t MCP4728;
    uint8_t new_address = 1;
    MCP4728->LDACPort = LDAC_GPIO_Port;
    MCP4728->LDACPin = LDAC_Pin;
    // initialize dac
    DAC_Init(&hi2c, &MCP4728, new_address);
    // assume the fixed resistor is the lower resistor
    voltage = Fix_Resistance / (resistance+Fix_Resistance) * 16;

    if (DAC_SendSignal(&hi2c, &MCP4728, voltage) == HAL_OK)
    {
        BattThermistorStatus = 1;
        sendCAN_PDU_MessageStatus();
    }
    
}

