#include "eeprom.h"

HAL_StatusTypeDef EEPROM_Init(void)
{
    return HAL_I2C_IsDeviceReady(&hi2c1,
                                 EEPROM_I2C_ADDRESS,
                                 3,
                                 100);
}

HAL_StatusTypeDef EEPROM_Erase(void)
{
    uint8_t erase_buffer[EEPROM_PAGE_SIZE];
    HAL_StatusTypeDef status;

    /* Fill buffer with erased value */
    memset(erase_buffer, 0x00, sizeof(erase_buffer));

    /* Erase EEPROM page by page */
    for (uint16_t address = 0; address < EEPROM_SIZE_BYTES;
         address += EEPROM_PAGE_SIZE)
    {
        status = HAL_I2C_Mem_Write(&hi2c1,
                                  EEPROM_I2C_ADDRESS,
                                  address,
                                  I2C_MEMADD_SIZE_8BIT,
                                  erase_buffer,
                                  EEPROM_PAGE_SIZE,
                                  100);

        if (status != HAL_OK)
        {
            return status;
        }

        /* Wait for EEPROM internal write cycle to finish */
        status = HAL_I2C_IsDeviceReady(&hi2c1,
                                       EEPROM_I2C_ADDRESS,
                                       10,
                                       10);

        if (status != HAL_OK)
        {
            return status;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef EEPROM_Write(uint8_t address,
                               const uint8_t *data,
                               uint16_t length)
{
    HAL_StatusTypeDef status;
    uint16_t bytes_to_write;

    if (data == NULL || length == 0U)
    {
        return HAL_ERROR;
    }

    if (((uint16_t)address + length) > EEPROM_SIZE_BYTES)
    {
        return HAL_ERROR;
    }

    while (length > 0U)
    {
        /* Calculate bytes remaining in the current page */
        bytes_to_write = EEPROM_PAGE_SIZE -
                         (address % EEPROM_PAGE_SIZE);

        if (bytes_to_write > length)
        {
            bytes_to_write = length;
        }

        status = HAL_I2C_Mem_Write(&hi2c1,
                                   EEPROM_I2C_ADDRESS,
                                   address,
                                   I2C_MEMADD_SIZE_8BIT,
                                   (uint8_t *)data,
                                   bytes_to_write,
                                   100);

        if (status != HAL_OK)
        {
            return status;
        }

        /* Wait for the EEPROM write cycle to complete */
        status = HAL_I2C_IsDeviceReady(&hi2c1,
                                       EEPROM_I2C_ADDRESS,
                                       10,
                                       10);

        if (status != HAL_OK)
        {
            return status;
        }

        /* Advance to the next chunk */
        address += bytes_to_write;
        data += bytes_to_write;
        length -= bytes_to_write;
    }

    return HAL_OK;
}

HAL_StatusTypeDef EEPROM_Read(uint8_t address,
                              uint8_t *data,
                              uint16_t length)
{
    if (data == NULL || length == 0U)
    {
        return HAL_ERROR;
    }

    if (((uint16_t)address + length) > EEPROM_SIZE_BYTES)
    {
        return HAL_ERROR;
    }

    return HAL_I2C_Mem_Read(&hi2c1,
                            EEPROM_I2C_ADDRESS,
                            address,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            length,
                            100);
}