/*
 * userCan.h
 *
 *  Created on: Mar 29, 2015
 *      Author: KabooHahahein
 */

#ifndef USER_CAN_F7_H_
#define USER_CAN_f7_H_

#include "bsp.h"
#include "stdbool.h"

HAL_StatusTypeDef F7_canInit(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef F7_canStart(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef F7_sendCanMessage(int id, int length, uint8_t *data);

#ifdef CHARGER_CAN_HANDLE
HAL_StatusTypeDef F7_sendCanMessageCharger(int id, int length, uint8_t *data);
#endif

#endif /* USER_CAN_F7_H_ */
