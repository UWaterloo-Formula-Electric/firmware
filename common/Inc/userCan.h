/*
 * userCan.h
 *
 *  Created on: Mar 29, 2015
 *      Author: KabooHahahein
 */

#ifndef USER_CAN_H_
#define USER_CAN_H_

#include "bsp.h"
#include "stdbool.h"
#include "FreeRTOS.h"
#include "task.h"
#define BOARD_DISABLE_CAN

// This allows automatically including the autogen can header file for the
// board specified in the Makefile
#define STRINGIZE_AUX(a) #a
#define STRINGIZE(a) STRINGIZE_AUX(a)
#define CAT_AUX(a, b) a##b
#define CAT(a, b) CAT_AUX(a, b)
// Use like: #include AUTOGEN_HEADER_NAME(BOARD_NAME), where BOARD_NAME is
// defined in the makefile
#define AUTOGEN_HEADER_NAME(boardName) STRINGIZE(CAT(boardName, _can.h))
#define AUTOGEN_DTC_HEADER_NAME(boardName) STRINGIZE(CAT(boardName, _dtc.h))

HAL_StatusTypeDef canInit(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef canStart(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef sendCanMessage(uint32_t id, uint32_t length, uint8_t *data);
HAL_StatusTypeDef sendDTCMessage(uint32_t dtcCode, int severity, uint64_t data);
#ifdef CHARGER_CAN_HANDLE
HAL_StatusTypeDef sendCanMessageCharger(uint32_t id, int length, uint8_t *data);
#endif

#endif /* USER_CAN_H_ */
