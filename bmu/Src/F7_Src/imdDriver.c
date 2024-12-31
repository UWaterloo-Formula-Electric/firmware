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

QueueHandle_t ImdDataHandle;
ImdData_s *pImdData;

void initImdMeasurements() {
    memset(pImdData, 0, sizeof(ImdData_s));
    vTaskDelay(1000);
}

ImdData_s * getImdData(){
    return pImdData;
}

void updateImdData(ImdData_s *ImdData) {
    memcpy(pImdData, ImdData, sizeof(ImdData_s));
}
