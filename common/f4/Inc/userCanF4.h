/*
 * userCan.h
 *
 *  Created on: Mar 29, 2015
 *      Author: KabooHahahein
 */

#ifndef USER_CAN_F4_H_
#define USER_CAN_F4_H_

#include "bsp.h"
#include "stdbool.h"

HAL_StatusTypeDef F4_canInit(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef F4_canStart(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef F4_sendCanMessage(int id, int length, uint8_t *data);

#endif /* USER_CAN_F4_H_ */
