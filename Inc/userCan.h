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
#include "freertos.h"
#include "task.h"

// This allows automatically including the autogen can header file for the
// board specified in the Makefile
#define STRINGIZE_AUX(a) #a
#define STRINGIZE(a) STRINGIZE_AUX(a)
#define CAT_AUX(a, b) a##b
#define CAT(a, b) CAT_AUX(a, b)
// Use like: #include AUTOGEN_HEADER_NAME(BOARD_NAME), where BOARD_NAME is
// defined in the makefile
#define AUTOGEN_HEADER_NAME(boardName) STRINGIZE(CAT(boardName, _can.h))

HAL_StatusTypeDef canInit(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef canStart(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef sendCanMessage(int id, int length, uint8_t *data);
HAL_StatusTypeDef sendCanMessageUrgent(int id, int length, uint8_t *data);
HAL_StatusTypeDef sendDTCMessage(int dtcCode, int severity, uint64_t data);
void canTask(const void *pvParameters);
void setCanTaskHandle(TaskHandle_t handle);
//bool sendCanMessage(const uint16_t id, const uint8_t *data, const uint8_t length);
//bool sendCanMessageTimeoutMs(const uint16_t id, const uint8_t *data,
                             //const uint8_t length, const uint32_t timeout);

#endif /* USER_CAN_H_ */
