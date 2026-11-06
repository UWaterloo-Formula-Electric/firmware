#include "chargerI2c.h"
#include "chargerRegs.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "task.h"


#define BQ_READ_TIMEOUT_MS  2 // max wait time for I2C read operation 
#define BQ_RETRY_DELAY_MS   2 // delay between retries for I2C read operation
#define BQ_READ_RETRIES     5 // number of retries for I2C read operation
#define BQ_WRITE_TIMEOUT_MS 2 // max wait time for I2C write operation

// function to read a register from charger IC 
HAL_StatusTypeDef chargerReadRegister(uint8_t reg, uint8_t *data){
    HAL_StatusTypeDef status;
    for (int i = 0; i < BQ_READ_RETRIES; i++){
        status = HAL_I2C_Mem_Read(
            &hi2c2, BQ_I2C_HAL_ADDRESS, reg, 
            I2C_MEMADD_SIZE_8BIT, data, 1, BQ_READ_TIMEOUT_MS
        );
        if (status == HAL_OK){
            return status;
        }
        if (i < BQ_READ_RETRIES - 1) {
            vTaskDelay(pdMS_TO_TICKS(BQ_RETRY_DELAY_MS)); // wait before retrying
        }
    }
    return status; 
}
// function to write a register to charger IC
HAL_StatusTypeDef chargerWriteRegister(uint8_t reg, uint8_t data){
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Write(
        &hi2c2, BQ_I2C_HAL_ADDRESS, reg, 
        I2C_MEMADD_SIZE_8BIT, &data, 1, BQ_WRITE_TIMEOUT_MS
    );
    return status;
}

HAL_StatusTypeDef chargerModifyBits(uint8_t reg, uint8_t mask, uint8_t value){
    uint8_t data = 0;
    HAL_StatusTypeDef readStatus = chargerReadRegister(reg, &data);
    if (readStatus != HAL_OK){
        return readStatus; // return if read failed
    }
    HAL_StatusTypeDef writeStatus;
    
    data = (data & ~mask) | (value & mask); // clear the bits specified by the mask and set them to the new value
    
    writeStatus = chargerWriteRegister(reg, data);
    return writeStatus;
}
