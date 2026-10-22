#include "chargeri2c.h"
#include "i2c.h"
#include "chargerRegs.h"

#define BQ_READ_TIMEOUT_MS  10

// function to read a register from charger IC 
HAL_StatusTypeDef chargerReadRegister(uint8_t reg, uint8_t *data){
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Read( 
        &hi2c2, BQ_I2C_HAL_ADDRESS, reg, 
        I2C_MEMADD_SIZE_8BIT, data, 1, BQ_READ_TIMEOUT_MS
    );
    return status;
}