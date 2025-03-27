#include "main.h"
#include "RelayPDUOutput.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_can.h"


extern QueueHandle_t pdu_hil_queue;
// data type thats auot genereated by the python file
struct BattThermistor {
    uint64_t BattThermistor : 18;
    uint64_t FILLER_END : 6;
};

// DAC Pin define, we use the first one for now
#define DAC1_LDAC_Port GPIOF
#define DAC1_LDAC_Pin 8

// resistors, change when we have the thermisotr voltage divider set up ready
#define Fix_Resistance 10000



BattThermistor can_rx;
uint32_t resistance = 0;
// to get the first 18 btis 
uint8_t mask = 0x3FFFF;
I2C_HandleTypeDef *hi2c

