#ifndef DAC_H
#define DAC_H

// Note that the unique address of dac can range from 1 to 8

//the 8 MSB on the DAC6551 is constant
//the power down mode connects a 100kOhm pulldow resistor to Vout
#define I2C_Port GPIOB
#define I2C_SCL_Pin 8
#define I2C_SDA_Pin 9

// set SDA/SCL high for bit banging
#define SDA_ON (I2C_Port->ODR |= (1<<I2C_SDA_Pin));
#define SCL_ON (I2C_Port->ODR |= (1<<I2C_SCL_Pin));
// set them low
#define SDA_OFF (I2C_Port->ODR &= ~(1<<I2C_SDA_Pin));
#define SCL_OFF (I2C_Port->ODR &=~(1<<I2C_SCL_Pin));
#define SDA_SetIn (I2C_Port->MODER &= ~(0x11 << (2 * I2C_SDA_Pin))); // clear the bits, set it as input  
//LDAC
#define LDAC_HIGH (config->LDACPort->ODR |= (1 << config->LDACPin))
#define LDAC_LOW (config->Port->ODR &= ~(1 << config->Pin))


// DAC specs
#define DAC_PWRDWN 1
#define DAC_FASTWR
#define DAC_Wakeup 0x09
#define DAC_CMD_SetPower 0xA0
#define DAC_CMD_SetRVoltage 0x8F
#define DAC_CMD_SendSingleSignal 0xB
#define DAC_CMD_SetGain 0xCF
#define DAC_Default_Add 0x60 << 1
#define Device_Code 0x0C
#define SendMask 0x80

// big banging
#define I2C_SDA_FallTime 300
#define I2C_SCL_FallTime 300
#define I2C_SDA_RiseTime 1000
#define I2C_SDA_RiseTime 1000
#define I2C_DataInputTime 250 // time u need to wait after changing the state of sda, before changing scl
#define I2C_DataHoldTime 3450 // time for which u need to ensure nothing happens, after setting scl to high
#define I2C_ClockLow 4700 // same as start conditio set up
#define I2C_ClockHigh 4000 // min time scl needs to stay high
#define I2C_StartingTime 4700
#define I2C_StartSetUp 4700
#define I2C_StartHoldTime 4000


HAL_StatusTypeDef DAC_Init(I2C_HandleTypeDef *hi2c, DAC_Config config);
HAL_StatusTyepDef DAC_SetVoltage(I2C_HandleTypeDef *hi2c, DAC_Config );
HAL_StatusTypeDef DAC_UpdateAdd(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef DAC_SendSignal(I2C_HandleTypeDef *hi2c, DAC_Config config, uint8_t channel);
Void SetRefVoltage(I2C_HandleTypeDef *hi2c, DAC_Config config);
void I2C_Start(); // to update teh address of the dacs, need bit bang. 
void I2C_Stop();
void I2C_SendByte();
void acknowledge();
uint8_t I2C_ReadBit();


typedef struct 
{
    uint8_t Address; // 3 bits, A0 A1 A2, shift to left 
    GPIO_TypeDef *LDACPort;
    uint16_t LDACPin; // port and pin for the LDAC pin
    Channel_t channels[4];
} DAC_t; // config for which dac device


typedef struct 
{
    uint16_t VoltageOut;
    // to check whether or not the 
    bool status;
} Channel_t;


#endif/*DAC_H*/
