/*
 * userCan.h
 *
 *  Created on: Mar 29, 2015
 *      Author: KabooHahahein
 */

#ifndef USER_CAN_H_
#define USER_CAN_H_

#include "stm32f7xx_hal.h"
#include "stdbool.h"

void canInit(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef canStartReceiving(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef sendCanMessage(int id, int length, uint8_t *data);
//bool sendCanMessage(const uint16_t id, const uint8_t *data, const uint8_t length);
//bool sendCanMessageTimeoutMs(const uint16_t id, const uint8_t *data,
                             //const uint8_t length, const uint32_t timeout);

#endif /* USER_CAN_H_ */
