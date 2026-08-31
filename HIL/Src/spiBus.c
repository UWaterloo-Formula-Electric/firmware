/**
  *****************************************************************************
  * @file    spiBus.c
  * @brief   Generic transfer wrappers for the bench SPI buses
  * @details SPI4 and SPI5 are broken out to the bench connectors. Both are
  * masters using hardware NSS, so there is no chip select to drive here.
  * Device specific framing belongs in that device's own file, which should
  * call these rather than the HAL directly.
  *****************************************************************************
  */

#include "spiBus.h"

#include "bsp.h"
#include "debug.h"

#define SPI_TIMEOUT_MS 15

static SPI_HandleTypeDef *getSpiHandle(SpiBus_t bus)
{
    switch (bus) {
        case SPI_BUS_4:
            return &SPI_4_HANDLE;
        case SPI_BUS_5:
            return &SPI_5_HANDLE;
        default:
            return NULL;
    }
}

HAL_StatusTypeDef spiTransfer(SpiBus_t bus, uint8_t *txData, uint8_t *rxData, uint16_t length)
{
    SPI_HandleTypeDef *handle = getSpiHandle(bus);

    if (handle == NULL || txData == NULL || rxData == NULL) {
        ERROR_PRINT("Failed to transfer on SPI bus %d, bad argument\n", bus);
        return HAL_ERROR;
    }

    if (HAL_SPI_TransmitReceive(handle, txData, rxData, length, SPI_TIMEOUT_MS) != HAL_OK) {
        ERROR_PRINT("Failed to transfer %u bytes on SPI bus %d\n", length, bus);
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef spiWrite(SpiBus_t bus, uint8_t *txData, uint16_t length)
{
    SPI_HandleTypeDef *handle = getSpiHandle(bus);

    if (handle == NULL || txData == NULL) {
        ERROR_PRINT("Failed to write on SPI bus %d, bad argument\n", bus);
        return HAL_ERROR;
    }

    if (HAL_SPI_Transmit(handle, txData, length, SPI_TIMEOUT_MS) != HAL_OK) {
        ERROR_PRINT("Failed to write %u bytes on SPI bus %d\n", length, bus);
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef spiRead(SpiBus_t bus, uint8_t *rxData, uint16_t length)
{
    SPI_HandleTypeDef *handle = getSpiHandle(bus);

    if (handle == NULL || rxData == NULL) {
        ERROR_PRINT("Failed to read on SPI bus %d, bad argument\n", bus);
        return HAL_ERROR;
    }

    if (HAL_SPI_Receive(handle, rxData, length, SPI_TIMEOUT_MS) != HAL_OK) {
        ERROR_PRINT("Failed to read %u bytes on SPI bus %d\n", length, bus);
        return HAL_ERROR;
    }

    return HAL_OK;
}
