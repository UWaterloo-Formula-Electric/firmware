#include "charger.h"
#include "controlStateMachine.h"
#include "chargerI2c.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "state_machine.h"

//verifying youre talking to the right chip
bqReadStatus_t chargerVerifyChip(uint8_t *data){
    HAL_StatusTypeDef status = chargerReadRegister(BQ_REG_PART_INFO, data);

    uint8_t partInfoData = (*data & BQ_PART_NUM_MASK) >> BQ_PART_NUM_SHIFT; // shift and mask to get part number
    
    if (status == HAL_ERROR || status == HAL_BUSY || status == HAL_TIMEOUT){
        return CHARGER_BUS_ERROR; //retried in 5 tries, at least 1 ms apart, and was unable to read
    }

    if (partInfoData != BQ_PART_NUMBER_EXPECTED){
        return CHARGER_WRONG_CHIP; // wrong chip ID read, mark the charger as unavailalbe, release CE
    }

    if(status == HAL_OK && partInfoData == (BQ_PART_NUMBER_EXPECTED)){
        return CHARGER_OK; // chip is verified
    }

    return CHARGER_BUS_ERROR; // default case, should not happen
}  