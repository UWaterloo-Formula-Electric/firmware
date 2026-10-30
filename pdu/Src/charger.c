#include "charger.h"
#include "controlStateMachine.h"
#include "chargerI2c.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "state_machine.h"

//verifying youre talking to the right chip
bqReadStatus_t chargerVerifyChip(uint8_t *data){
    if (data == NULL){
        return CHARGER_INVALID_ARG; //invalid pointer
    }
    HAL_StatusTypeDef status = chargerReadRegister(BQ_REG_PART_INFO, data);

    if (status != HAL_OK){
        return CHARGER_BUS_ERROR; //retried in 5 tries, at least 1 ms apart, and was unable to read
    }

    uint8_t partInfoData = chargerDecodePartNumber(*data);

    if (partInfoData != BQ_PART_NUMBER_EXPECTED){
        return CHARGER_WRONG_CHIP; // wrong chip ID read, mark the charger as unavailalbe, release CE
    }
    return CHARGER_OK;   
}  

uint8_t chargerDecodeRevision(uint8_t data){ //pass by val since its small and we don't want to worry abotu null ptrs
    return (data & BQ_DEV_REV_MASK) >> BQ_DEV_REV_SHIFT; // shift and mask to get revision number
}

uint8_t chargerDecodePartNumber(uint8_t data){
    return (data & BQ_PART_NUM_MASK) >> BQ_PART_NUM_SHIFT; // shift and mask to get part number
}

