#ifndef RELAYPDUOUTPUT_H
#define RELAYPDUOUTPUT_H

#include "main.h"
#include "RelayPDUOutput.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_can.h"



CAN_RxHeaderTypeDef rx_msg;
uint32_t OutputTaskID = 2281901827;
uint8_t data[2];

#endif