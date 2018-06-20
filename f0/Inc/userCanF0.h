/*
 * userCan.h
 *
 *  Created on: Mar 29, 2015
 *      Author: KabooHahahein
 */

#ifndef USER_CAN_F0_H_
#define USER_CAN_F0_H_

#include "bsp.h"
#include "stdbool.h"

HAL_StatusTypeDef F0_canInit(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef F0_canStartReceiving(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef F0_sendCanMessage(int id, int length, uint8_t *data);

#endif /* USER_CAN_F0_H_ */
