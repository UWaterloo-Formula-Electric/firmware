#include "chargerI2c.h"
#include "chargerRegs.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "task.h"


#define BQ_READ_TIMEOUT_MS  2 // max wait time for I2C read operation 
#define BQ_RETRY_DELAY_MS   2 // delay between retries for I2C read operation
#define BQ_READ_RETRIES     5 // number of retries for I2C read operation
#define BQ_WRITE_TIMEOUT_MS 2 // max wait time for I2C write operation

// shared by the 8 and 16 bit reads, retries on bus failure
static HAL_StatusTypeDef chargerReadBytes(uint8_t reg, uint8_t *buf, uint16_t len){
    HAL_StatusTypeDef status;
    for (int i = 0; i < BQ_READ_RETRIES; i++){
        status = HAL_I2C_Mem_Read(
            &hi2c2, BQ_I2C_HAL_ADDRESS, reg,
            I2C_MEMADD_SIZE_8BIT, buf, len, BQ_READ_TIMEOUT_MS
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

// shared by the 8 and 16 bit writes, single attempt
static HAL_StatusTypeDef chargerWriteBytes(uint8_t reg, uint8_t *buf, uint16_t len){
    return HAL_I2C_Mem_Write(
        &hi2c2, BQ_I2C_HAL_ADDRESS, reg,
        I2C_MEMADD_SIZE_8BIT, buf, len, BQ_WRITE_TIMEOUT_MS
    );
}

HAL_StatusTypeDef chargerReadRegister(uint8_t reg, uint8_t *data){
    return chargerReadBytes(reg, data, 1);
}

HAL_StatusTypeDef chargerWriteRegister(uint8_t reg, uint8_t data){
    return chargerWriteBytes(reg, &data, 1);
}

// one transaction so the chip never holds a half updated value
HAL_StatusTypeDef chargerRead16BitRegister(uint8_t reg, uint16_t *data){
    uint8_t buf[2];
    HAL_StatusTypeDef status = chargerReadBytes(reg, buf, 2);
    if (status != HAL_OK){
        return status; // leave *data untouched on failure
    }
    *data = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8); // little endian per datasheet: LSB at reg, MSB at reg + 1
    return HAL_OK;
}

HAL_StatusTypeDef chargerWrite16BitRegister(uint8_t reg, uint16_t data){
    uint8_t buf[2] = { data & 0xFF, data >> 8 }; // little endian per datasheet: LSB at reg, MSB at reg + 1
    return chargerWriteBytes(reg, buf, 2);
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
