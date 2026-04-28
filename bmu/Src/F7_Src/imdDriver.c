/**
  *****************************************************************************
  * @file    imdDriver.c
  * @author  Richard Matthews
  * @brief   Module containing functions to read IMD status
  * @details The IMD (Bender ISO175C-32-SS) is an insulation monitoring
  * device which monitors the resistance between the HV Bus and the chassis
  * ground. It reports a fault if the insulation drop below a certain value.
  * The IMD communicates with the BMU through a gpio pin indicating a boolean
  * fault status (OK or FAIL) and the CAN bus.
  *
  ******************************************************************************
  */

#include <string.h>
#include "imdDriver.h"
#include "bsp.h"
#include "debug.h"
#include "bmu_dtc.h"
#include "state_machine.h"
#include "controlStateMachine.h"
#include "bmu_can.h"

#define IMD_SENSE_PIN_FAULT    GPIO_PIN_RESET
#define IMD_SENSE_PIN_NO_FAULT GPIO_PIN_SET

#define IMD_REQUEST_UNUSED_BYTE 0xFFU
#define IMD_WRITE_LOCK_INDEX 0x6BU
#define IMD_WRITE_ENABLE_VALUE 0xFCU
#define IMD_WRITE_DISABLE_VALUE 0xFDU
#define IMD_THRESHOLD_ERROR_GET_INDEX 0x46U
#define IMD_THRESHOLD_ERROR_SET_INDEX 0x47U
#define IMD_REQUEST_SPACING_MS 120U

QueueHandle_t ImdDataHandle;
ImdData_s *pImdData;

static void imdSetUnusedRequestBytes() {
    IMD_Request_Data1 = IMD_REQUEST_UNUSED_BYTE;
    IMD_Request_Data2 = IMD_REQUEST_UNUSED_BYTE;
}

static void imdPrepareRequest(uint8_t index) {
    IMD_Request_Index = index;
    imdSetUnusedRequestBytes();
}

static HAL_StatusTypeDef imdSendRequest(uint8_t index) {
    imdPrepareRequest(index);
    return sendCAN_IMD_Request();
}

void initImdMeasurements() {
    memset(pImdData, 0, sizeof(ImdData_s));
    vTaskDelay(3000);
}

ImdData_s * getImdData(){
    return pImdData;
}

void updateImdData(ImdData_s *ImdData) {
    memcpy(pImdData, ImdData, sizeof(ImdData_s));
}

HAL_StatusTypeDef imdSetIsolationThresholdError(uint16_t thresholdKohm) {
    if (thresholdKohm < IMD_ISOLATION_THRESHOLD_ERROR_MIN_KOHM ||
        thresholdKohm > IMD_ISOLATION_THRESHOLD_ERROR_MAX_KOHM) {
        return HAL_ERROR;
    }

    imdPrepareRequest(IMD_WRITE_LOCK_INDEX);
    IMD_Request_Data1 = IMD_WRITE_ENABLE_VALUE;
    HAL_StatusTypeDef status = sendCAN_IMD_Request();
    if (status != HAL_OK) {
        return status;
    }

    vTaskDelay(pdMS_TO_TICKS(IMD_REQUEST_SPACING_MS));

    imdPrepareRequest(IMD_THRESHOLD_ERROR_SET_INDEX);
    IMD_Request_Data1 = thresholdKohm & 0xFFU;
    IMD_Request_Data2 = (thresholdKohm >> 8) & 0xFFU;
    status = sendCAN_IMD_Request();
    if (status != HAL_OK) {
        return status;
    }

    vTaskDelay(pdMS_TO_TICKS(IMD_REQUEST_SPACING_MS));

    imdPrepareRequest(IMD_WRITE_LOCK_INDEX);
    IMD_Request_Data1 = IMD_WRITE_DISABLE_VALUE;
    return sendCAN_IMD_Request();
}

HAL_StatusTypeDef imdRequestIsolationThresholdError() {
    IMD_Response_Index = IMD_REQUEST_UNUSED_BYTE;
    return imdSendRequest(IMD_THRESHOLD_ERROR_GET_INDEX);
}

bool imdGetIsolationThresholdError(uint16_t *thresholdKohm) {
    if (thresholdKohm == NULL || IMD_Response_Index != IMD_THRESHOLD_ERROR_GET_INDEX) {
        uint8_t responseIndex = IMD_Response_Index;
        ERROR_PRINT("Response Index is not as expected: 0x%x\r\n", responseIndex);
        return false;
    }

    *thresholdKohm = IMD_Response_Data1 | (IMD_Response_Data2 << 8);
    return *thresholdKohm >= IMD_ISOLATION_THRESHOLD_ERROR_MIN_KOHM &&
           *thresholdKohm <= IMD_ISOLATION_THRESHOLD_ERROR_MAX_KOHM;
}
