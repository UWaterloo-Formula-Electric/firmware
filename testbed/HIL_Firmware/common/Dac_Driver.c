#include <stdio.h>
#include "dac.h"
#include "driver/dac_oneshot.h"


void Timer_Delay(uint8_t ns)
{
    // our oscillator drives the timer at 8M Hz, so each tick is 125ns
    // we need 37.6 ticks
    TIM_HandleTypeDef htim1;
    htim1.Instance = TIM1;
    uint16_t ticks = us / 125;
    __HAL_TIM_SET_COUNTER(&htim1, 0)
    while(__HAL_TIM_GET_COUNTER(&htim1) < ticks);
}

void I2C_Stop()
{
    SDA_OFF
    Timer_Delay(I2C_ClockLow);
    SCL_ON
    Timer_Delay(I2C_ClockHigh); // should be the same as the starting
    SDA_ON
    Timer_Delay(I2C_DataInputTime);
    SCL_OFF
    Timer_Delay(I2C_ClockLow);
}

void I2C_Start()
{
    SDA_ON
    SCL_ON
    Timer_Delay(I2C_ClockHigh);
    // pull down SDA
    SDA_OFF
    Timer_Delay(I2C_StartSetUp);
    SCL_OFF
    Timer_Delay(I2C_StartHoldTime); // need to hold it for this long after starting condition
}

void I2C_SendByte(uint8_t byte)
{
    // if bit is 1
    for(int i{0}; i < 8; i++)
    {
        if(byte&SendMask)
        {
            SDA_ON
            Timer_Delay(I2C_DataInputTime); // wait for sda to stabilize
            SCL_ON // sample data
            Timer_Delay(I2C_ClockHigh);
        } 
        else
        {
            SDA_OFF
            Timer_Delay(I2C_DataInputTime);
            SCL_ON
            Timer_Delay(I2C_ClockHigh);
        }
        byte <<= 1;
    }
    SCL_OFF
    Timer_Delay(I2C_ClockLow);
}

uint8_t I2C_ReadBit()
{
    SCL_OFF
    Timer_Delay(I2C_ClockLow);
    SDA_SetIn
    Timer_Delay(I2C_DataInputTime);
    SCL_ON
    Timer_Delay(I2C_ClockHigh);
    uint8_t bit = (I2C_Port->IDR & (1<<I2C_SDA_Pin));
    SCL_OFF;
    Timer_Delay(I2C_ClockLow);
    return bit;
}


void acknowledge()
{
    // acknolwedge bit
    SCL_ON
    Timer_Delay(I2C_ClockHigh);
    uint8_t ack = I2C_ReadBit();
    if(ack==1){
        // bad condition
        I2C_Stop();
        return
    }
    SCL_OFF
}

// new address is 3 bit
HAL_StatusTypeDef DAC_UpdateAdd(I2C_HandleTypeDef *hi2c, DAC_t *config, uint8_t new_address)
{
    // if sent a command to the common address, all devices will be 
    I2C_Start(); // SCL is low
    LDAC_HIGH // set ldac high
    uint8_t full_address = (Device_Code<<4) | config->address;
    uint8_t mask = 0x80;
    
    // address
    I2C_SendByte(mask&full_address); 
    acknowledge();

    uint8_t command = 0x3 << 5;
    command |= ((config->address) << 1);
    command |= 1;
    // command
    I2C_SendByte(command);

    // need to set ldac pin low from high, before the scl line goes high for the acknowledgement bit
    LDAC_LOW // turn it low
    acknowledge();

    // new address is A0, A1, A2, 3 bits
    new_address <<= 2;
    new_address |= 0x3<<5;
    new_address |= 2
    I2C_SendByte(new_address);
    acknowledge();

    // confirmation
    new_address |= 1;
    I2C_SendByte(new_address)
    I2C_Stop();
}

// always set to internal reference. use gain to scale it up/down
HAL_StatusTypeDef SetRefVoltage(I2C_HandleTypeDef *hi2c, DAC_t *config)
{
    uint8_t SetVoltage = DAC_CMD_SetRVoltage;
    uint8_t full_address = (Device_Code << 4) | config->Address;
    return HAL_I2C_Master_Transmit(hi2c, full_address, &SetVoltage, sizeof(SetVoltage), HAL_MAX_DELAY);
}

// set all gain to 2
HAL_StatusTypeDef SetGain(I2C_HandleTypeDef *hi2c, DAC_t *config)
{
    uint8_t SetGain = DAC_CMD_SetGain;
    uint8_t full_address = (Device_Code << 4) | config->Address;
    return HAL_I2C_Master_Transmit(hi2c, full_address, &SetGain, sizeof(SetVoltage), HAL_MAX_DELAY);
}
// might be useless lol. whatever
HAL_StatusTypeDef SetPower(I2C_HandleTypeDef *hi2c, DAC_t *config)
{
    uint8_t PwrDwnNum = 4 - config->NumSignal;
    uint8_t data[2];
    uint8_t command = DAC_CMD_SetPower; // for setting power mode
    uint8_t secondbyte = 0x00;
    uint8_t mask = 0x10; // start masking from the end, 
    for(uint8_t i = 1; i <= 2 && PwrDwnNum > 0; i++)
    {
        secondbyte |= mask;
        mask <<= 2;
        PwrDwnNum--;
    }
    mask = 1;

    for(uint8_t i = 0; i < PwrDwnNum; i++) 
    {  
        command |= mask;
        mask <<= 2;
    }

    data[0] = command;
    data[1] = secondbyte;
    uint8_t full_address = (Device_Code << 4) | config->Address;
    return HAL_I2C_Master_Transmit(hi2c, full_address, &data, sizeof(data), HAL_MAX_DELAY);
}

// use single write command, make sure ldac is low
HAL_StatusTypeDef DAC_SendSignal(I2C_HandleTypeDef *hi2c, DAC_t *config, uint16_t voltage)
{
    // convert the desired output into digital code value for dac
    // with vref = 2.048, gain=2
    voltage *= 1000;
    int channel = -1;
    // find the first available channel
    for(int i = 0; i < 4; i++)
    {
        if(config->channels[i]->status==0)
        {
            channel=i;
            break;
        }
    }
    
    if(channel==-1)
    {
        //debug print, no channel is available
    }

    // update the config
    config->channels[channel]->status=1;
    config->channels[channel]->VoltageOut=voltage;


    uint8_t data[3];
    uint8_t command = DAC_CMD_SendSingleSignal<<3;
    command |= (channel<<1);
    
    data[0] = command;
    // high byte
    data[1] = (voltage >> 8) & 0xf;
    // Vref, power mode, gain selection bits
    data[1] |= 0x90;
    // low byte
    data[2] = voltage & 0xff;
    uint8_t full_address = (Device_Code << 4) | config->Address;
    return HAL_I2C_Master_Transmit(hi2c, full_address, &data, sizeof(data), HAL_MAX_DELAY);
}

// 
HAL_StatusTypeDef DAC_Init(I2C_HandleTypeDef *hi2c, DAC_t *config, uint8_t new_address)
{
    // default address
    // waking up every dac
    uint8_t wakeup = DAC_Wakeup;
    HAL_I2C_Master_Transmit(hi2c, 0x00, &wakeup, sizeof(wakeup), HAL_MAX_DELAY);
    LDAC_LOW
    // update the address
    DAC_UpdateAdd(&hi2c, &config, new_address);
    uint8_t data[8];
    uint8_t full_address = (Device_Code << 4) | config->Address;
    for(int i=0; i < 8; i++)
    {
        data[i] = 0x00; // send all zero, to set the dac output to be zero at the beginning
    }

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, full_address, &data, sizeof(data), HAL_MAX_DELAY);
    if(status != HAL_OK)
    {
        return status;
    }

    status = SetRefVoltage(hi2c, config);
    if(status != HAL_OK)
    {
        return status;
    }
    return HAL_OK;
}
